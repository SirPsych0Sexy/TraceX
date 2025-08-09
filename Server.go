package main

import (
	"fmt"
	"io"
	"log"
	"net/http"
	"os"
)

// Clave secreta para el cifrado/descifrado. DEBE ser la misma en el cliente y el servidor.
const xorKey = "1b4a4445504fa017b51e7f2824254f0a"

// xorCipher aplica el cifrado/descifrado XOR a un slice de bytes.
// Modifica el slice de datos directamente (in-place).
func xorCipher(data []byte, key string) {
	keyBytes := []byte(key)
	keyLen := len(keyBytes)
	for i := 0; i < len(data); i++ {
		data[i] = data[i] ^ keyBytes[i%keyLen]
	}
}

func dataHandler(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Método no permitido", http.StatusMethodNotAllowed)
		return
	}

	// 1. Leer el cuerpo de la petición (datos cifrados)
	encryptedBody, err := io.ReadAll(r.Body)
	if err != nil {
		http.Error(w, "Error al leer el cuerpo de la petición", http.StatusInternalServerError)
		return
	}
	defer r.Body.Close()

	if len(encryptedBody) == 0 {
		http.Error(w, "Cuerpo de la petición vacío", http.StatusBadRequest)
		return
	}

	// 2. ¡NUEVO! Descifrar los datos usando la clave XOR
	// La función modifica el slice 'encryptedBody' directamente, convirtiéndolo en datos descifrados.
	xorCipher(encryptedBody, xorKey)
	decryptedData := encryptedBody // Renombramos por claridad

	// 3. Abrir el archivo en modo "append"
	file, err := os.OpenFile("received_data.log", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		http.Error(w, "Error interno del servidor", http.StatusInternalServerError)
		log.Printf("Error abriendo el archivo: %v", err)
		return
	}
	defer file.Close()

	// 4. Escribir los datos ya descifrados en el archivo
	if _, err := file.Write(append(decryptedData, '\n')); err != nil {
		http.Error(w, "Error interno del servidor", http.StatusInternalServerError)
		log.Printf("Error escribiendo en el archivo: %v", err)
		return
	}

	log.Printf("Datos recibidos y descifrados correctamente: %s", string(decryptedData))
	fmt.Fprintf(w, "Datos recibidos y procesados con éxito.")
}

func main() {
	http.HandleFunc("/data", dataHandler)
	port := ":8080"
	log.Printf("Servidor escuchando en http://localhost%s/data", port)
	if err := http.ListenAndServe(port, nil); err != nil {
		log.Fatalf("No se pudo iniciar el servidor: %s\n", err)
	}
}

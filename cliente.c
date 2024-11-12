#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Función para enviar datos al servidor
void enviar_datos(const char *ip, int puerto, const char *mensaje) {
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Creamos el socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: No se pudo crear el socket\n");
        return;
    }

    // Configuramos la dirección del servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(puerto);
    
    // Convertimos la IP del servidor a formato de red
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        printf("Dirección inválida o no soportada\n");
        return;
    }

    // Intentamos conectar al servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error: Conexión fallida\n");
        return;
    }

    // Enviamos el mensaje
    send(sock, mensaje, strlen(mensaje), 0);
    printf("Mensaje enviado: %s\n", mensaje);

    // Cerramos el socket
    close(sock);
}

int main() {
    const char *ip = "127.0.0.1";
    int puerto = 8080;

    while (1) {
        // Ejecuta agente y captura su salida en un buffer para enviar al servidor
        system("./agente");  // Ejecuta el programa agente
        enviar_datos(ip, puerto, "Datos de monitoreo");  // Envía datos al servidor
        sleep(60);  // Intervalo de ejecución (60 segundos)
    }

    return 0;
}

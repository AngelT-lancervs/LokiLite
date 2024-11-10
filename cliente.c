#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "10.0.2.15"  // IP fija del servidor

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <servicio1> <servicio2> ... <intervalo>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Dirección IP del servidor y número de servicios
    int num_services = argc - 2;
    int interval = atoi(argv[argc - 1]);
    char **services = &argv[1];

    // Crear socket de cliente
    int client_socket;
    struct sockaddr_in server_addr;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }

    // Configurar dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convertir IP del servidor de texto a binario
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Dirección no válida");
        exit(EXIT_FAILURE);
    }

    // Conectar al servidor
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar");
        exit(EXIT_FAILURE);
    }

    // Construir el mensaje con los servicios y el intervalo
    char message[1024];
    strcpy(message, "Servicios: ");
    for (int i = 0; i < num_services; i++) {
        strcat(message, services[i]);
        strcat(message, " ");
    }
    char interval_str[50];
    sprintf(interval_str, "Intervalo: %d segundos", interval);
    strcat(message, interval_str);

    // Enviar el mensaje al servidor
    send(client_socket, message, strlen(message), 0);

    // Recibir respuesta del servidor
    char buffer[1024];
    int n = recv(client_socket, buffer, sizeof(buffer), 0);
    if (n > 0) {
        buffer[n] = '\0';  // Null-terminate the received message
        printf("Respuesta del servidor: %s\n", buffer);
    }

    // Cerrar la conexión
    close(client_socket);

    return 0;
}

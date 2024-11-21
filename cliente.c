// cliente.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"   // IP fija del servidor
#define SERVER_PORT 8080       // Puerto fijo del servidor
#define MAX_LOGS 5              // Máximo de logs recientes por servicio

// Función para enviar el dashboard al servidor
void send_dashboard_to_server(int socket_fd, char *dashboard) {
    int len = strlen(dashboard);
    send(socket_fd, &len, sizeof(len), 0);  // Enviar el tamaño del mensaje
    send(socket_fd, dashboard, len, 0);    // Enviar el contenido del dashboard
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2> ... <intervalo_segundos>\n", argv[0]);
        exit(1);
    }

    // El último parámetro es el intervalo de tiempo
    int interval = atoi(argv[argc - 1]);

    // El número de servicios es el total de parámetros menos el intervalo
    int num_services = argc - 2;
    char **services = &argv[1];

    // Crear socket para comunicarse con el servidor
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Conectar con el servidor
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al conectar al servidor");
        close(socket_fd);
        exit(1);
    }

    while (1) {
        // Construir el comando para ejecutar `agente.c` con todos los servicios
        char command[2048];
        snprintf(command, sizeof(command), "./agente");

        // Agregar los servicios al comando
        for (int i = 0; i < num_services; i++) {
            snprintf(command + strlen(command), sizeof(command) - strlen(command), " %s", services[i]);
        }

        // Ejecutar agente.c y obtener el dashboard
        FILE *fp = popen(command, "r");
        if (fp == NULL) {
            perror("Error al ejecutar agente.c");
            close(socket_fd);
            exit(1);
        }

        // Leer la salida de `agente.c` y almacenarla en el dashboard
        char dashboard[1024 * 1024];  // Asumimos un tamaño máximo para el dashboard
        fread(dashboard, 1, sizeof(dashboard), fp);
        fclose(fp);

        // Enviar el dashboard al servidor
        send_dashboard_to_server(socket_fd, dashboard);

        // Esperar el intervalo antes de la próxima ejecución
        sleep(interval);
    }

    close(socket_fd);
    return 0;
}

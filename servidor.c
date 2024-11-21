#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080        // Puerto en el que el servidor escucha
#define MAX_DASHBOARD_SIZE 1024 * 1024  // Máximo tamaño del dashboard recibido
#define GENERAL_THRESHOLD 10    // Umbral general para el total de logs

// Estructura para almacenar la cantidad de logs por prioridad
typedef struct {
    int emerg, alert, crit, err, warn, notice, info, debug;
} LogCounts;

// Función para analizar el dashboard recibido
void parse_dashboard_and_check_threshold(char *dashboard) {
    char *line = strtok(dashboard, "\n");
    while (line != NULL) {
        // Buscar la línea con los servicios
        if (strncmp(line, "Servicio: ", 10) == 0) {
            // Extraemos el nombre del servicio
            char *service_name = line + 10;
            printf("\nServicio: %s\n", service_name);
            LogCounts counts = {0};

            // Leer las prioridades y contar los logs
            while ((line = strtok(NULL, "\n")) != NULL) {
                if (strncmp(line, "EMERG:", 6) == 0) {
                    sscanf(line, "EMERG:\t\t\t%d", &counts.emerg);
                } else if (strncmp(line, "ALERT:", 6) == 0) {
                    sscanf(line, "ALERT:\t\t\t%d", &counts.alert);
                } else if (strncmp(line, "CRIT:", 5) == 0) {
                    sscanf(line, "CRIT:\t\t\t%d", &counts.crit);
                } else if (strncmp(line, "ERR:", 4) == 0) {
                    sscanf(line, "ERR:\t\t\t%d", &counts.err);
                } else if (strncmp(line, "WARN:", 5) == 0) {
                    sscanf(line, "WARN:\t\t\t%d", &counts.warn);
                } else if (strncmp(line, "NOTICE:", 7) == 0) {
                    sscanf(line, "NOTICE:\t\t\t%d", &counts.notice);
                } else if (strncmp(line, "INFO:", 5) == 0) {
                    sscanf(line, "INFO:\t\t\t%d", &counts.info);
                } else if (strncmp(line, "DEBUG:", 6) == 0) {
                    sscanf(line, "DEBUG:\t\t\t%d", &counts.debug);
                }

                // Mostrar las prioridades
                if (strncmp(line, "Últimos", 7) == 0) {
                    break;  // Llegamos a la sección de últimos logs, por lo que terminamos de procesar
                }
            }

            // Sumar el total de logs para el servicio
            int total_logs = counts.emerg + counts.alert + counts.crit + counts.err + counts.warn +
                             counts.notice + counts.info + counts.debug;

            // Mostrar el dashboard para este servicio
            printf("----------------------------------------------------\n");
            printf("Prioridades:\n");
            printf("EMERG: %d\n", counts.emerg);
            printf("ALERT: %d\n", counts.alert);
            printf("CRIT: %d\n", counts.crit);
            printf("ERR: %d\n", counts.err);
            printf("WARN: %d\n", counts.warn);
            printf("NOTICE: %d\n", counts.notice);
            printf("INFO: %d\n", counts.info);
            printf("DEBUG: %d\n", counts.debug);
            printf("Total de logs: %d\n", total_logs);
            printf("----------------------------------------------------\n");

            // Verificar si el total de logs supera el umbral general
            if (total_logs > GENERAL_THRESHOLD) {
                printf("¡UMBRAL GENERAL SUPERADO! Se enviará notificación por WhatsApp.\n");
                // Aquí iría el código para enviar el mensaje por WhatsApp
            }
        }
        line = strtok(NULL, "\n");
    }
}

int main() {
    // Crear socket para recibir conexiones de clientes
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    // Configurar dirección del servidor
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Asociar el socket con la dirección
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al asociar el socket");
        close(server_fd);
        exit(1);
    }

    // Escuchar conexiones entrantes
    if (listen(server_fd, 5) < 0) {
        perror("Error al escuchar el socket");
        close(server_fd);
        exit(1);
    }

    printf("Esperando conexiones en el puerto %d...\n", SERVER_PORT);

    // Aceptar conexión del cliente
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        perror("Error al aceptar la conexión");
        close(server_fd);
        exit(1);
    }

    printf("Cliente conectado desde %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Recibir el dashboard del cliente
    char dashboard[MAX_DASHBOARD_SIZE];
    int len;
    recv(client_fd, &len, sizeof(len), 0);
    recv(client_fd, dashboard, len, 0);

    // Procesar y verificar el dashboard recibido
    parse_dashboard_and_check_threshold(dashboard);

    // Cerrar la conexión con el cliente
    close(client_fd);
    close(server_fd);

    return 0;
}

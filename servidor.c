#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curl/curl.h>

#define SERVER_PORT 1234       // Puerto en el que el servidor escucha
#define MAX_DASHBOARD_SIZE 1024 * 1024  // Tamaño máximo del dashboard recibido
#define GENERAL_THRESHOLD 20   // Umbral general para el total de logs

// Estructura para almacenar la cantidad de logs por prioridad
typedef struct {
    int emerg, alert, crit, err, warn, notice, info, debug;
} LogCounts;

// Estructura para registrar si se ha notificado el umbral para cada servicio
typedef struct {
    char service_name[256];  // Nombre del servicio
    int umbral_notificado;  // 0 si no se ha notificado, 1 si ya se notificó
} ServiceNotificationStatus;

// Función para enviar notificación por WhatsApp usando Twilio
void enviar_notificacion_whatsapp(const char *service_name) {
    // Crear el comando para ejecutar el script Python con el nombre del servicio como argumento
    char command[512];
    snprintf(command, sizeof(command), "python3 enviar_mensaje.py \"%s\"", service_name);

    // Ejecutar el comando
    int result = system(command);

    // Verificar si hubo un error al ejecutar el comando
    if (result == -1) {
        perror("Error al ejecutar el script de WhatsApp");
    } else {
        printf("Notificación enviada a WhatsApp para el servicio: %s\n", service_name);
    }
}

// Función para analizar el dashboard recibido y verificar umbrales
void parse_dashboard_and_check_threshold(char *dashboard, ServiceNotificationStatus *notification_status, int *notification_count) {
    char *line = strtok(dashboard, "\n");

    while (line != NULL) {
        if (strncmp(line, "Servicio: ", 10) == 0) {
            char *service_name = line + 10;
            printf("\nServicio: %s\n", service_name);
            LogCounts counts = {0};

            // Extraer los log counts
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
                if (strncmp(line, "Últimos", 7) == 0) {
                    break;
                }
            }

            int total_logs = counts.emerg + counts.alert + counts.crit + counts.err +
                             counts.warn + counts.notice + counts.info + counts.debug;

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

            // Si el total de logs supera el umbral y no se ha enviado la notificación para este servicio
            if (total_logs > GENERAL_THRESHOLD) {
                int already_notified = 0;
                // Comprobar si ya se ha enviado la notificación para este servicio
                for (int i = 0; i < *notification_count; i++) {
                    if (strcmp(notification_status[i].service_name, service_name) == 0 &&
                        notification_status[i].umbral_notificado == 1) {
                        already_notified = 1;
                        break;
                    }
                }

                if (!already_notified) {
                    // Enviar notificación por WhatsApp
                    printf("¡UMBRAL GENERAL SUPERADO en %s! Se enviará notificación por WhatsApp.\n", service_name);
                    enviar_notificacion_whatsapp(service_name);

                    // Registrar que se ha enviado la notificación para este servicio
                    strcpy(notification_status[*notification_count].service_name, service_name);
                    notification_status[*notification_count].umbral_notificado = 1;
                    (*notification_count)++;
                }
            }
        }
        line = strtok(NULL, "\n");
    }
}

// Función para borrar logs usando journalctl
void borrar_logs_journalctl() {
    printf("Borrando logs de journalctl...\n");
    system("sudo journalctl --rotate");
    system("sudo journalctl --vacuum-time=1s");
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error al crear el socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al asociar el socket");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Error al escuchar el socket");
        close(server_fd);
        exit(1);
    }

    printf("Esperando conexiones en el puerto %d...\n", SERVER_PORT);
    borrar_logs_journalctl();

    ServiceNotificationStatus notification_status[100];  // Array para guardar el estado de las notificaciones
    int notification_count = 0;  // Contador de servicios para saber cuántos servicios han sido notificados

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

    while (1) {
        char dashboard[MAX_DASHBOARD_SIZE];
        int len;

        memset(dashboard, 0, sizeof(dashboard));

        recv(client_fd, &len, sizeof(len), 0);
        recv(client_fd, dashboard, len, 0);

        system("clear");

        printf("\nDashboard recibido desde el cliente:\n");
        parse_dashboard_and_check_threshold(dashboard, notification_status, &notification_count);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}

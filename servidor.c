#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_PORT 8080       // Puerto del servidor
#define MAX_LOG_SIZE 4096      // Tamaño máximo del log recibido

// Función para enviar la alerta por WhatsApp (implementación pendiente)
void enviar_alerta_whatsapp(const char *mensaje) {
    // Aquí se debe incluir el código para enviar el mensaje a través de WhatsApp
    // Este código dependería de la API o la librería que uses para el envío
    printf("Enviando alerta de WhatsApp: %s\n", mensaje);
}

// Función para analizar los logs y verificar si se superan los umbrales
void analizar_logs_y_enviar_alerta(const char *data) {
    int info_count = 0, notice_count = 0;
    int emerg_count = 0, err_count = 0, warn_count = 0, crit_count = 0, alert_count = 0;

    // Copiar los datos para usar strtok, ya que strtok modifica la cadena
    char *data_copy = strdup(data);  // Copia de la cadena

    // Analizamos la cadena de logs para contar las ocurrencias de cada tipo
    char *line = strtok(data_copy, "\n");  // Usamos strtok en la copia de los datos
    while (line != NULL) {
        if (strstr(line, "INFO")) info_count++;
        else if (strstr(line, "NOTICE")) notice_count++;
        else if (strstr(line, "EMERG")) emerg_count++;
        else if (strstr(line, "ERR")) err_count++;
        else if (strstr(line, "WARN")) warn_count++;
        else if (strstr(line, "CRIT")) crit_count++;
        else if (strstr(line, "ALERT")) alert_count++;

        line = strtok(NULL, "\n");  // Obtener la siguiente línea de log
    }

    // Liberar la memoria de la copia
    free(data_copy);

    // Verificar si alguna de las condiciones de umbral se cumple
    if ((info_count + notice_count > 100) || emerg_count > 0 || err_count > 0 || warn_count > 0 || crit_count > 0 || alert_count > 0) {
        char mensaje[1024];
        snprintf(mensaje, sizeof(mensaje), "Alerta: Se han superado los umbrales de logs!\n"
                                          "INFO: %d, NOTICE: %d, EMERG: %d, ERR: %d, WARN: %d, CRIT: %d, ALERT: %d",
                                          info_count, notice_count, emerg_count, err_count, warn_count, crit_count, alert_count);
        enviar_alerta_whatsapp(mensaje);  // Enviar alerta por WhatsApp
    }
}

// Función para manejar la conexión con el cliente
void manejar_conexion(int client_socket) {
    char buffer[MAX_LOG_SIZE];

    // Recibir los datos del cliente
    int bytes_recibidos = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_recibidos < 0) {
        perror("Error recibiendo datos");
        close(client_socket);
        return;
    }

    buffer[bytes_recibidos] = '\0';  // Asegurar que la cadena esté terminada en nulo

    printf("Datos recibidos:\n%s\n", buffer);

    // Analizar los logs y enviar alerta si es necesario
    analizar_logs_y_enviar_alerta(buffer);

    close(client_socket);  // Cerrar la conexión con el cliente
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Crear el socket del servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creando el socket");
        exit(1);
    }

    // Configurar la dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Aceptar conexiones de cualquier dirección
    server_addr.sin_port = htons(SERVER_PORT);

    // Enlazar el socket con la dirección
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al enlazar el socket");
        close(server_socket);
        exit(1);
    }

    // Escuchar conexiones entrantes
    if (listen(server_socket, 5) < 0) {
        perror("Error al escuchar en el socket");
        close(server_socket);
        exit(1);
    }

    printf("Esperando conexiones en el puerto %d...\n", SERVER_PORT);

    while (1) {
        // Aceptar una conexión entrante
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Error al aceptar conexión");
            continue;
        }

        printf("Conexión aceptada.\n");

        // Manejar la conexión con el cliente
        manejar_conexion(client_socket);
    }

    close(server_socket);  // Cerrar el socket del servidor
    return 0;
}

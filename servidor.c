#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_LOG_SIZE 1024

// Estructura para los parámetros del programa
struct client_args {
    char *program_name; // Nombre del programa a ejecutar
    int interval;       // Intervalo de ejecución en segundos
};

// Función que maneja la conexión de un cliente, recibiendo y enviando logs
void *client_handler(void *socket_desc) {
    int client_socket = *((int *)socket_desc);
    char message[MAX_LOG_SIZE];

    // Espera a recibir los servicios y el intervalo desde el cliente
    int n = recv(client_socket, message, sizeof(message), 0);
    if (n <= 0) {
        perror("Error al recibir mensaje del cliente");
        close(client_socket);
        pthread_exit(NULL);
    }

    // Mostrar lo que se recibió (servicios e intervalo)
    printf("Mensaje recibido del cliente: %s\n", message);

    // Ejecutar el programa (agente) que genera los logs
    FILE *fp;
    char buffer[MAX_LOG_SIZE];
    char output[8192] = "";  // Buffer para almacenar los logs generados

    // Ejecutar el programa cada 'intervalo' segundos y enviar los logs al cliente
    while (1) {
        fp = popen(message, "r"); // Ejecutar el programa
        if (fp == NULL) {
            perror("Error al ejecutar el programa");
            break;
        }

        // Leer los logs del programa y almacenarlos
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            strcat(output, buffer);
        }
        pclose(fp);

        // Enviar los logs al cliente
        send(client_socket, output, strlen(output), 0);
        
        // Limpiar el buffer de logs para la próxima iteración
        memset(output, 0, sizeof(output));
        
        // Esperar el intervalo antes de ejecutar de nuevo
        sleep(5);  // Intervalo en segundos, puedes cambiar el valor dinámicamente si es necesario
    }

    // Cerrar el socket
    close(client_socket);
    pthread_exit(NULL);
}

// Función para ejecutar el servidor y aceptar conexiones de clientes
void *execute_program(void *args) {
    struct client_args *params = (struct client_args *)args;
    char *program = params->program_name;
    int interval = params->interval;

    // Crear el socket del servidor
    int server_socket, client_socket[MAX_CLIENTS], new_socket;
    struct sockaddr_in server_addr, client_addr;
    int opt = 1;
    int addrlen = sizeof(server_addr);

    // Crear el socket del servidor
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error al crear socket del servidor");
        exit(EXIT_FAILURE);
    }

    // Configurar opciones del socket
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("Error al configurar opciones del socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Asignar la dirección y puerto al socket del servidor
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error al asignar dirección al socket del servidor");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Error al escuchar conexiones");
        exit(EXIT_FAILURE);
    }

    printf("Servidor esperando conexiones...\n");

    // Aceptar conexiones de los clientes
    while (1) {
        int client_addr_len = sizeof(client_addr);
        if ((new_socket = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len)) < 0) {
            perror("Error al aceptar la conexión");
            exit(EXIT_FAILURE);
        }

        // Crear un hilo para manejar la conexión con el cliente
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, (void *)&new_socket);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <programa_a_ejecutarse> <intervalo_en_segundos>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parámetros para ejecutar el programa
    char *program_to_execute = argv[1];
    int interval = atoi(argv[2]);

    if (interval <= 0) {
        printf("El intervalo debe ser mayor a 0\n");
        return EXIT_FAILURE;
    }

    // Estructura con los parámetros
    struct client_args args = {program_to_execute, interval};

    // Crear un hilo para ejecutar el servidor
    pthread_t tid;
    pthread_create(&tid, NULL, execute_program, (void *)&args);

    // Esperar a que termine el hilo del servidor
    pthread_join(tid, NULL);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>  // Incluir para usar wait()

#define SERVER_IP "127.0.0.1"   // IP fija del servidor
#define SERVER_PORT 1234      // Puerto fijo del servidor
#define MAX_DASHBOARD_SIZE 1024 * 1024  // Máximo tamaño del dashboard recibido

// Función para enviar el dashboard al servidor
void send_dashboard_to_server(int socket_fd, char *dashboard) {
    int len = strlen(dashboard);
    send(socket_fd, &len, sizeof(len), 0);  // Enviar el tamaño del mensaje
    send(socket_fd, dashboard, len, 0);    // Enviar el contenido del dashboard
}

// Función para ejecutar agente.c usando fork y execv
int execute_agent(char **args, char *dashboard, size_t max_size) {
    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
        perror("Error al crear el pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error al hacer fork");
        return -1;
    } else if (pid == 0) {
        // Proceso hijo
        close(pipe_fd[0]);          // Cerrar lectura del pipe
        dup2(pipe_fd[1], STDOUT_FILENO); // Redirigir stdout al pipe
        close(pipe_fd[1]);          // Cerrar el descriptor original del pipe

        execv("./agente", args);    // Ejecutar agente
        perror("Error al ejecutar execv"); // Solo si execv falla
        exit(1);
    } else {
        // Proceso padre
        close(pipe_fd[1]);          // Cerrar escritura del pipe
        ssize_t total_bytes_read = 0;
        while (total_bytes_read < max_size - 1) {
            ssize_t bytes_read = read(pipe_fd[0], dashboard + total_bytes_read, max_size - 1 - total_bytes_read);
            if (bytes_read < 0) {
                perror("Error al leer del pipe");
                close(pipe_fd[0]);
                return -1;
            }
            if (bytes_read == 0) {
                break;  // Fin de la salida del comando
            }
            total_bytes_read += bytes_read;
        }
        dashboard[total_bytes_read] = '\0'; // Terminar la cadena
        close(pipe_fd[0]);
        wait(NULL);  // Esperar a que el proceso hijo termine
    }
    return 0;
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
        // Preparar los argumentos para execv
        char *args[num_services + 2];
        args[0] = "./agente";
        for (int i = 0; i < num_services; i++) {
            args[i + 1] = services[i];
        }
        args[num_services + 1] = NULL;

        // Ejecutar agente y obtener el dashboard
        char dashboard[MAX_DASHBOARD_SIZE];  // Aseguramos un tamaño suficiente para el dashboard
        if (execute_agent(args, dashboard, sizeof(dashboard)) < 0) {
            fprintf(stderr, "Error al ejecutar agente\n");
            close(socket_fd);
            exit(1);
        }

        // Mostrar el dashboard en el cliente
        printf("\nDashboard generado por el cliente:\n");
        printf("-------------------------------------\n");
        printf("%s\n", dashboard);  // Mostrar el dashboard antes de enviarlo

        // Enviar el dashboard al servidor
        send_dashboard_to_server(socket_fd, dashboard);

        // Esperar el intervalo antes de la próxima ejecución
        sleep(interval);
    }

    close(socket_fd);
    return 0;
}

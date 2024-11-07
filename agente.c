#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define DEFAULT_INTERVAL 5  // Intervalo predeterminado en segundos si no se proporciona uno

// Declaramos un mutex para controlar el acceso al dashboard
pthread_mutex_t log_mutex;
char **services;   // Lista de servicios proporcionada por el usuario
int num_services;  // Número de servicios a monitorear
int interval;      // Intervalo de tiempo entre actualizaciones

// Estructura para almacenar el conteo de logs por prioridad para cada servicio
typedef struct {
    int emerg;
    int alert;
    int crit;
    int err;
    int warn;
    int notice;
    int info;
    int debug;
} LogCounts;

LogCounts *log_counts;  // Array para almacenar conteos por servicio

// Función para inicializar variables y estructuras
void initialize(int argc, char *argv[]) {
    interval = (argc > 2) ? atoi(argv[argc - 1]) : DEFAULT_INTERVAL;  // Intervalo
    num_services = argc - 2;  // Número de servicios a monitorear
    services = &argv[1];      // Array de servicios proporcionados por el usuario

    // Inicializamos el array de conteos y el mutex
    log_counts = malloc(num_services * sizeof(LogCounts));
    memset(log_counts, 0, num_services * sizeof(LogCounts));
    pthread_mutex_init(&log_mutex, NULL);
}

// Función que ejecuta journalctl y cuenta logs según la prioridad para un servicio específico
void monitor_service_logs(int service_index) {
    char *service = services[service_index];  // Nombre del servicio
    LogCounts *counts = &log_counts[service_index];  // Contadores del servicio

    int fd[2];  // Descriptores de la tubería
    pipe(fd);   // Crear la tubería para la comunicación entre procesos

    if (fork() == 0) {  // Proceso hijo
        close(fd[0]);   // Cerrar el extremo de lectura de la tubería
        dup2(fd[1], STDOUT_FILENO);  // Redirigir la salida estándar al extremo de escritura de la tubería

        // Ejecutar journalctl con los parámetros deseados
        execlp("journalctl", "journalctl", "-u", service, "-p", "0..7", "--no-pager", NULL);
        perror("Error ejecutando journalctl");  // En caso de error
        exit(1);  // Terminar el proceso hijo si hay un error
    } else {  // Proceso padre
        close(fd[1]);  // Cerrar el extremo de escritura de la tubería
        FILE *fp = fdopen(fd[0], "r");  // Convertir el descriptor de archivo a FILE*

        // Leer cada línea y actualizar los contadores de logs acumulativos
        char line[1024];
        while (fgets(line, sizeof(line), fp) != NULL) {
            pthread_mutex_lock(&log_mutex);  // Bloqueamos el mutex para actualizar los contadores

            // Mostrar los logs y actualizar los contadores
            if (strstr(line, "emerg")) {
                counts->emerg++;
                printf("EMERG: %s", line);  // Mostrar log de prioridad EMERG
            } else if (strstr(line, "alert")) {
                counts->alert++;
                printf("ALERT: %s", line);  // Mostrar log de prioridad ALERT
            } else if (strstr(line, "crit")) {
                counts->crit++;
                printf("CRIT: %s", line);  // Mostrar log de prioridad CRIT
            } else if (strstr(line, "err")) {
                counts->err++;
                printf("ERROR: %s", line);  // Mostrar log de prioridad ERROR
            } else if (strstr(line, "warn")) {
                counts->warn++;
                printf("WARN: %s", line);  // Mostrar log de prioridad WARN
            } else if (strstr(line, "notice")) {
                counts->notice++;
                printf("NOTICE: %s", line);  // Mostrar log de prioridad NOTICE
            } else if (strstr(line, "info")) {
                counts->info++;
                printf("INFO: %s", line);  // Mostrar log de prioridad INFO
            } else if (strstr(line, "debug")) {
                counts->debug++;
                printf("DEBUG: %s", line);  // Mostrar log de prioridad DEBUG
            }

            pthread_mutex_unlock(&log_mutex);  // Liberamos el mutex
        }
        
        fclose(fp);  // Cerramos el archivo
        wait(NULL);  // Esperamos a que el proceso hijo termine
    }
}

// Función que será ejecutada por cada hilo para monitorear un servicio
void *monitor_service(void *arg) {
    int service_index = *(int *)arg;  // Índice del servicio en la lista
    while (1) {
        monitor_service_logs(service_index);  // Ejecutar el monitoreo de logs
        sleep(interval);  // Esperar el intervalo antes de la siguiente iteración
    }
    return NULL;
}

// Función para mostrar el dashboard en la consola
void display_dashboard() {
    system("clear");  // Limpia la pantalla para una visualización más ordenada
    printf("------ Dashboard de Logs por Prioridad ------\n");

    // Mostrar conteos acumulados de cada servicio
    for (int i = 0; i < num_services; i++) {
        printf("Servicio: %s\n", services[i]);
        printf("EMERG: %d | ALERT: %d | CRIT: %d | ERR: %d | WARN: %d | NOTICE: %d | INFO: %d | DEBUG: %d\n",
               log_counts[i].emerg, log_counts[i].alert, log_counts[i].crit, log_counts[i].err,
               log_counts[i].warn, log_counts[i].notice, log_counts[i].info, log_counts[i].debug);
        printf("--------------------------------------------\n");
    }
}

// Función para iniciar y manejar el dashboard en tiempo real
void start_dashboard() {
    while (1) {
        pthread_mutex_lock(&log_mutex);  // Bloqueamos el mutex antes de mostrar el dashboard
        display_dashboard();
        pthread_mutex_unlock(&log_mutex);  // Liberamos el mutex
        sleep(interval);  // Esperar el intervalo antes de la próxima actualización
    }
}

// Función para limpiar recursos al finalizar
void cleanup() {
    pthread_mutex_destroy(&log_mutex);  // Destruir el mutex
    free(log_counts);  // Liberar la memoria asignada a los contadores
}

int main(int argc, char *argv[]) {
    // Verificación y configuración inicial
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2> ... [intervalo]\n", argv[0]);
        exit(1);
    }
    initialize(argc, argv);  // Inicializar variables y estructuras

    pthread_t threads[num_services];  // Array de hilos para monitorear cada servicio
    int service_indices[num_services];  // Array de índices para pasar a cada hilo

    // Crear un hilo para cada servicio
    for (int i = 0; i < num_services; i++) {
        service_indices[i] = i;  // Asignar el índice correspondiente al servicio
        pthread_create(&threads[i], NULL, monitor_service, (void *)&service_indices[i]);
    }

    // Iniciar el dashboard en el proceso principal
    start_dashboard();

    // Esperar a que todos los hilos terminen
    for (int i = 0; i < num_services; i++) {
        pthread_join(threads[i], NULL);
    }

    cleanup();  // Limpiar recursos al finalizar
    return 0;
}

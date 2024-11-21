#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_LOGS 5 // Máximo de logs recientes a mostrar por servicio

pthread_mutex_t log_mutex;
char **services;
int num_services;

typedef struct {
    int emerg, alert, crit, err, warn, notice, info, debug;
} LogCounts;

typedef struct {
    char logs[MAX_LOGS][1024];
    int log_count;
    int next_index;
} LogBuffer;

LogCounts *log_counts;
LogBuffer *log_buffers;

void initialize(int argc, char *argv[]) {
    num_services = argc - 1;
    services = &argv[1];

    log_counts = malloc(num_services * sizeof(LogCounts));
    log_buffers = malloc(num_services * sizeof(LogBuffer));
    memset(log_counts, 0, num_services * sizeof(LogCounts));
    memset(log_buffers, 0, num_services * sizeof(LogBuffer));
    pthread_mutex_init(&log_mutex, NULL);
}

// Función que ejecuta journalctl y cuenta logs según la prioridad para un servicio específico
void monitor_service_logs(int service_index) {
    char *service = services[service_index];
    LogCounts *counts = &log_counts[service_index];
    LogBuffer *buffer = &log_buffers[service_index];

    int fd[2];
    pipe(fd);

    if (fork() == 0) {  // Proceso hijo
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        execlp("journalctl", "journalctl", "-u", service, "-p", "0..7", "--no-pager", NULL);
        perror("Error ejecutando journalctl");
        exit(1);
    } else {  // Proceso padre
        close(fd[1]);
        FILE *fp = fdopen(fd[0], "r");

        char line[1024];
        while (fgets(line, sizeof(line), fp) != NULL) {
            pthread_mutex_lock(&log_mutex);  // Bloqueo mutex antes de procesar logs

            // Comprobar prioridades
            if (strstr(line, "EMERG") || strstr(line, "<0>") || strstr(line, "<emerg>")) counts->emerg++;
            else if (strstr(line, "ALERT") || strstr(line, "<1>") || strstr(line, "<alert>") || strstr(line, "Accepted")) counts->alert++;
            else if (strstr(line, "CRIT") || strstr(line, "<2>") || strstr(line, "<crit>")) counts->crit++;
            else if (strstr(line, "ERR") || strstr(line, "<3>") || strstr(line, "<err>") || strstr(line, "error") || strstr(line, "Failed")) counts->err++;
            else if (strstr(line, "WARN") || strstr(line, "<4>") || strstr(line, "<warn>") ) counts->warn++;
            else if (strstr(line, "NOTICE") || strstr(line, "<5>") || strstr(line, "<notice>") || strstr(line, "Start")) counts->notice++;
            else if (strstr(line, "INFO") || strstr(line, "<6>") || strstr(line, "<info>")) counts->info++;
            else if (strstr(line, "DEBUG") || strstr(line, "<7>") || strstr(line, "<debug>")) counts->debug++;
            else { 
                // Si no hay coincidencias, asignar INFO por defecto
                counts->info++;
            }

            // Almacenar los últimos logs
            strncpy(buffer->logs[buffer->next_index], line, sizeof(buffer->logs[buffer->next_index]) - 1);
            buffer->logs[buffer->next_index][sizeof(buffer->logs[buffer->next_index]) - 1] = '\0';
            buffer->next_index = (buffer->next_index + 1) % MAX_LOGS;
            if (buffer->log_count < MAX_LOGS) buffer->log_count++;

            pthread_mutex_unlock(&log_mutex);
        }

        fclose(fp);
        wait(NULL);
    }
}

// Función para mostrar el dashboard de forma más clara
void display_dashboard() {
    system("clear");
    printf("------ Dashboard de Logs por Prioridad ------\n");

    for (int i = 0; i < num_services; i++) {
        LogCounts *counts = &log_counts[i];
        LogBuffer *buffer = &log_buffers[i];

        printf("\nServicio: %s\n", services[i]);
        printf("----------------------------------------------------\n");
        printf("Prioridad\t\tCantidad de Logs\n");
        printf("----------------------------------------------------\n");
        printf("EMERG:\t\t\t%d\n", counts->emerg);
        printf("ALERT:\t\t\t%d\n", counts->alert);
        printf("CRIT:\t\t\t%d\n", counts->crit);
        printf("ERR:\t\t\t%d\n", counts->err);
        printf("WARN:\t\t\t%d\n", counts->warn);
        printf("NOTICE:\t\t\t%d\n", counts->notice);
        printf("INFO:\t\t\t%d\n", counts->info);
        printf("DEBUG:\t\t\t%d\n", counts->debug);
        printf("----------------------------------------------------\n");

        printf("Últimos %d logs:\n", MAX_LOGS);
        int start = (buffer->next_index + MAX_LOGS - buffer->log_count) % MAX_LOGS;
        for (int j = 0; j < buffer->log_count; j++) {
            int index = (start + j) % MAX_LOGS;
            printf("%s", buffer->logs[index]);
        }
        printf("----------------------------------------------------\n");

        // Limpiar contadores y buffers después de mostrar
        memset(counts, 0, sizeof(LogCounts));
        memset(buffer->logs, 0, sizeof(buffer->logs));
        buffer->log_count = 0;
        buffer->next_index = 0;
    }
}

// Función para ejecutar el monitoreo una sola vez
void monitor_services_once() {
    for (int i = 0; i < num_services; i++) {
        monitor_service_logs(i);
    }
}

// Función para limpiar recursos al finalizar
void cleanup() {
    pthread_mutex_destroy(&log_mutex);
    free(log_counts);
    free(log_buffers);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2> ...\n", argv[0]);
        exit(1);
    }
    initialize(argc, argv);

    // Monitorear los logs una sola vez
    monitor_services_once();

    // Mostrar el dashboard
    display_dashboard();

    cleanup();
    return 0;
}

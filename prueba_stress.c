#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define MIN_SERVICES 2

// Estructura para pasar el nombre del servicio y el intervalo
typedef struct {
    char service_name[256];
    int interval;
} ServiceArgs;

// Función de estrés que reinicia el servicio en intervalos regulares
void *stress_service(void *arg) {
    ServiceArgs *service_args = (ServiceArgs *)arg;
    char command[300];
    snprintf(command, sizeof(command), "sudo systemctl restart %s", service_args->service_name);

    while (1) {
        printf("Reiniciando servicio: %s\n", service_args->service_name);
        int result = system(command);
        if (result == -1) {
            perror("Error al ejecutar comando de reinicio");
            pthread_exit(NULL);
        }
        sleep(service_args->interval);
    }
}

int main(int argc, char *argv[]) {
    if (argc < MIN_SERVICES + 2) {
        fprintf(stderr, "Uso: %s <servicio1> <servicio2> ... <intervalo_en_segundos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_services = argc - 2;
    int interval = atoi(argv[argc - 1]);

    if (interval <= 0) {
        fprintf(stderr, "El intervalo debe ser mayor a 0\n");
        exit(EXIT_FAILURE);
    }

    pthread_t threads[num_services];
    ServiceArgs service_args[num_services];

    for (int i = 0; i < num_services; i++) {
        strncpy(service_args[i].service_name, argv[i + 1], sizeof(service_args[i].service_name) - 1);
        service_args[i].service_name[sizeof(service_args[i].service_name) - 1] = '\0';
        service_args[i].interval = interval;

        if (pthread_create(&threads[i], NULL, stress_service, &service_args[i]) != 0) {
            perror("Error al crear hilo de estrés para el servicio");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_services; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}

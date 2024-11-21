#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Función para reiniciar los servicios SSH, Cron y NetworkManager
void *reiniciar_servicio(void *arg) {
    while (1) {
        // Reiniciar el servicio SSH
        printf("Reiniciando el servicio SSH...\n");
        system("sudo systemctl restart ssh");

        // Dormir 10 segundos
        sleep(10);

        // Reiniciar el servicio Cron
        printf("Reiniciando el servicio Cron...\n");
        system("sudo systemctl restart cron");

        // Dormir 10 segundos
        sleep(3);

        // Reiniciar el servicio NetworkManager
        printf("Reiniciando el servicio NetworkManager...\n");
        system("sudo systemctl restart NetworkManager");

        // Dormir 10 segundos
        sleep(10);
    }
}

int main() {
    pthread_t tid;
    int res;

    // Crear el hilo que alternará entre reiniciar SSH, Cron y NetworkManager
    res = pthread_create(&tid, NULL, reiniciar_servicio, NULL);
    if (res != 0) {
        perror("Error al crear el hilo");
        exit(EXIT_FAILURE);
    }

    // Esperar a que el hilo termine (en este caso, el hilo nunca terminará)
    pthread_join(tid, NULL);

    return 0;
}

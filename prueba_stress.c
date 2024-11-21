#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

void *generar_estrés_ssh(void *arg) {
    // Comando SSH fallido
    char *comando_ssh = "sshpass -p 'contraseñaincorrecta' ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null usuario@localhost exit";
    for (int i = 0; i < 3; i++) {  // Ejecutar la prueba de estrés SSH 3 veces
        system(comando_ssh);  // Intento de conexión fallido
        sleep(1); // Ajusta el tiempo de espera si es necesario
    }
    return NULL;
}

void *generar_estrés_network_manager(void *arg) {
    char *interface = "eth0";  // Cambia esto si usas una interfaz diferente
    for (int i = 0; i < 3; i++) {  // Ejecutar la prueba de estrés en NetworkManager 3 veces
        // Desconectar la interfaz de red
        char comando_desconectar[256];
        snprintf(comando_desconectar, sizeof(comando_desconectar), "nmcli device disconnect %s", interface);
        system(comando_desconectar);  // Ejecutar el comando para desconectar la interfaz

        sleep(1); // Espera antes de reconectar la interfaz

        // Reconectar la interfaz de red
        char comando_conectar[256];
        snprintf(comando_conectar, sizeof(comando_conectar), "nmcli device connect %s", interface);
        system(comando_conectar);  // Ejecutar el comando para reconectar la interfaz

        sleep(1); // Ajusta el tiempo de espera si es necesario
    }
    return NULL;
}

int main() {
    pthread_t tid_ssh, tid_network_manager;

    // Alternar entre las pruebas de red y SSH indefinidamente
    while (1) {
        // Crear hilo para generar estrés en SSH
        int res_ssh = pthread_create(&tid_ssh, NULL, generar_estrés_ssh, NULL);
        if (res_ssh != 0) {
            perror("Error al crear el hilo SSH");
            exit(EXIT_FAILURE);
        }

        // Esperar que el hilo SSH termine
        pthread_join(tid_ssh, NULL);

        // Crear hilo para generar estrés en NetworkManager
        int res_network_manager = pthread_create(&tid_network_manager, NULL, generar_estrés_network_manager, NULL);
        if (res_network_manager != 0) {
            perror("Error al crear el hilo de NetworkManager");
            exit(EXIT_FAILURE);
        }

        // Esperar que el hilo de NetworkManager termine
        pthread_join(tid_network_manager, NULL);

        // Repetir el ciclo alternando entre las pruebas
    }

    return 0;
}

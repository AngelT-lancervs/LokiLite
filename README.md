# LokiLite

Este proyecto permite monitorear los logs de múltiples servicios del sistema en tiempo real, clasificar los logs según su prioridad y mostrar los últimos registros en un dashboard legible. También incluye scripts para pruebas de stress del sistema.

---

## **Compilación**
Utilice `make` para compilar el proyecto. Esto generará los binarios necesarios:

```bash
make
```

## **Uso**

Primero se ejecuta el servidor
```bash
./servidor
```

En otra instancia se ejecuta el cliente con dos o más servicios y un intervalo de tiempo en segundos
```bash
./cliente servicio1 servicio2 ... intervalo
```

Dependiendo del servicio, se puede ejecutar la prueba de estrés, siempre y cuando se trabaje con los servicios: ssh, cron y NetworkManager. Caso contrario, es necesario modificar este archivo para los servicios específicos a monitorear.
```bash
./prueba_stress
```

## **Consideraciones**
Es necesario modificar el archivo enviar_mensaje.py con los datos de la API Twilo, tanto el SID como el token.


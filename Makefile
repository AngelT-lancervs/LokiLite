# Definición de compilador y banderas
CC = gcc
CFLAGS = -lpthread -lcurl  # Agregar la biblioteca de curl

# Definición de nombres de los ejecutables
AGENT_EXEC = agente
SERVER_EXEC = servidor
STRESS_EXEC = prueba_stress
CLIENT_EXEC = cliente  # Nombre del ejecutable para el cliente

# Regla para compilar todos los ejecutables
all: $(AGENT_EXEC) $(SERVER_EXEC) $(STRESS_EXEC) $(CLIENT_EXEC)

# Regla para compilar agente
$(AGENT_EXEC): agente.c
	$(CC) agente.c -o $(AGENT_EXEC) $(CFLAGS)

# Regla para compilar servidor
$(SERVER_EXEC): servidor.c
	$(CC) servidor.c -o $(SERVER_EXEC) $(CFLAGS)

# Regla para compilar prueba de estrés
$(STRESS_EXEC): prueba_stress.c
	$(CC) prueba_stress.c -o $(STRESS_EXEC) $(CFLAGS)

# Regla para compilar cliente
$(CLIENT_EXEC): cliente.c
	$(CC) cliente.c -o $(CLIENT_EXEC) $(CFLAGS)

# Regla para limpiar ejecutables
clean:
	rm -f $(AGENT_EXEC) $(SERVER_EXEC) $(STRESS_EXEC) $(CLIENT_EXEC)

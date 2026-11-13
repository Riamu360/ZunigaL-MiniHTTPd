#ifndef SERVER_H
#define SERVER_H

void iniciar_servidor(const char *puerto);

/* Helper to create a passive TCP socket bound to the given service/port */
int passiveTCP(const char *service, int qlen);

/* Process a client request on the given fd */
void procesar_peticion(int client_fd);

#endif // SERVER_H
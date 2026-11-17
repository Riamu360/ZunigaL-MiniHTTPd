#ifndef SERVER_H
#define SERVER_H

/* Crea un socket TCP pasivo enlazado al servicio/puerto indicado */
int passiveTCP(const char *service, int qlen);

/* Procesa una petición HTTP del cliente en el descriptor de archivo (fd) dado.
   Devuelve 1 si la conexión debe mantenerse activa (persistente),
   o 0 si debe cerrarse. */
int procesar_peticion(int client_fd);

#endif

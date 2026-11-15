#ifndef SERVER_H
#define SERVER_H

/* Create a passive TCP socket bound to the given service/port */
int passiveTCP(const char *service, int qlen);

/* Process a client HTTP request on the given fd.
   Returns 1 if the connection should be kept alive (persistent),
   or 0 if it should be closed. */
int procesar_peticion(int client_fd);

#endif // SERVER_H

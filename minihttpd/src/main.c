#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "server.h"

#define QLEN 5
#define MAX_EVENTS 32

int main(int argc, char *argv[]) {
    const char *port = "80";
    if (argc > 1) {
        port = argv[1];
    }
    printf("El Servidor MiniHTTPd se está ejecutando en el puerto %s...\n", port);

    /* Crear socket TCP pasivo */
    int sd = passiveTCP(port, QLEN);
    if (sd < 0) {
        fprintf(stderr, "Error: no se pudo iniciar el servidor\n");
        exit(1);
    }

    /* Configurar el socket de escucha en modo no bloqueante */
    int flags = fcntl(sd, F_GETFL, 0);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);

    /* Crear instancia de epoll */
    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        close(sd);
        exit(1);
    }

    /* Registrar el socket de escucha en epoll */
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev) < 0) {
        perror("epoll_ctl add listen");
        close(sd);
        close(epfd);
        exit(1);
    }

    /* Bucle principal de eventos */
    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sd) {
                /* Nueva conexión entrante */
                struct sockaddr_storage clt_addr;
                socklen_t addr_len = sizeof(clt_addr);
                int nsd = accept(sd, (struct sockaddr *)&clt_addr, &addr_len);
                if (nsd >= 0) {
                    int cflags = fcntl(nsd, F_GETFL, 0);
                    fcntl(nsd, F_SETFL, cflags | O_NONBLOCK);
                    ev.events = EPOLLIN;
                    ev.data.fd = nsd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, nsd, &ev);
                }
            } else {
                /* Datos del cliente listos — procesar petición */
                int client_fd = events[i].data.fd;
                int keep_alive = procesar_peticion(client_fd);
                if (keep_alive) {
                    /* Conexión persistente: mantener el fd en epoll para la siguiente petición */
                    ev.events = EPOLLIN;
                    ev.data.fd = client_fd;
                    epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &ev);
                } else {
                    /* Cerrar conexión */
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }

    close(sd);
    close(epfd);
    return 0;
}

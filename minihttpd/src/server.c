#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include "server.h"
#include "http.h"

#define QLEN 5
#define MAX_EVENTS 32

extern int passiveTCP(const char *service, int qlen);

void iniciar_servidor(const char *puerto) {
    struct sockaddr_in cltAddr;
    socklen_t szAddr = sizeof(cltAddr);
    int sd, nsd;

    sd = passiveTCP(puerto, QLEN);
    if (sd < 0) {
        perror("Error en passiveTCP");
        exit(1);
    }

    // Socket servidor en modo no bloqueante para epoll
    int flags = fcntl(sd, F_GETFL, 0);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);

    int epfd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];

    ev.events = EPOLLIN;
    ev.data.fd = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);

    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == sd) {
                // Nueva conexion entrante
                nsd = accept(sd, (struct sockaddr *)&cltAddr, &szAddr);
                if (nsd >= 0) {
                    flags = fcntl(nsd, F_GETFL, 0);
                    fcntl(nsd, F_SETFL, flags | O_NONBLOCK);

                    ev.events = EPOLLIN;
                    ev.data.fd = nsd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, nsd, &ev);
                }
            } else {
                // Petición de un cliente existente
                int client_fd = events[i].data.fd;
                procesar_peticion(client_fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
                close(client_fd);
            }
        }
    }
    close(sd);
    close(epfd);
}
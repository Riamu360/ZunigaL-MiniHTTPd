#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <limits.h>

#include "server.h"
#include "http.h"
#include "mime.h"
#include "files.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define QLEN 5
#define MAX_EVENTS 32

int passiveTCP(const char *service, int qlen) {
    struct addrinfo hints, *res, *p;
    int sockfd = -1;
    int opt = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;      // bind to any address

    int rc = getaddrinfo(NULL, service, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0) break; // success
        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(res);

    if (sockfd < 0) {
        perror("socket bind");
        return -1;
    }
    if (listen(sockfd, qlen) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

void procesar_peticion(int client_fd) {
    const size_t REQ_BUF = 8192;
    char req[REQ_BUF];
    ssize_t n = recv(client_fd, req, REQ_BUF - 1, 0);
    if (n <= 0) return;
    req[n] = '\0';

    char method[16];
    char uri[256];
    char version[16];
    if (parse_http_request(req, method, sizeof(method), uri, sizeof(uri), version, sizeof(version)) != 0) {
        const char *msg = "Bad Request";
        char resp[512];
        int len = build_http_response(400, msg, "text/plain", msg, strlen(msg), resp, sizeof(resp));
        send(client_fd, resp, len, 0);
        return;
    }

    if (strcmp(method, "GET") != 0) {
        const char *msg = "Method Not Allowed";
        char resp[512];
        int len = build_http_response(405, msg, "text/plain", msg, strlen(msg), resp, sizeof(resp));
        send(client_fd, resp, len, 0);
        return;
    }

    off_t size = 0;
    char real_path[PATH_MAX];
    int fd = open_static_file(uri, &size, real_path, sizeof(real_path));
    if (fd < 0) {
        int code = (fd == -403) ? 403 : 404;
        const char *msg = (code == 403) ? "Forbidden" : "Not Found";
        char resp[512];
        int len = build_http_response(code, msg, "text/plain", msg, strlen(msg), resp, sizeof(resp));
        send(client_fd, resp, len, 0);
        return;
    }

    const char *mime = get_mime_type(real_path);
    char *body = malloc((size_t)size);
    if (!body) {
        close(fd);
        const char *msg = "Internal Server Error";
        char resp[512];
        int len = build_http_response(500, msg, "text/plain", msg, strlen(msg), resp, sizeof(resp));
        send(client_fd, resp, len, 0);
        return;
    }
    ssize_t read_bytes = read(fd, body, size);
    close(fd);
    if (read_bytes != size) {
        free(body);
        const char *msg = "Internal Server Error";
        char resp[512];
        int len = build_http_response(500, msg, "text/plain", msg, strlen(msg), resp, sizeof(resp));
        send(client_fd, resp, len, 0);
        return;
    }

    /* Build header only (no body) so it fits in a small buffer */
    char header[1024];
    int hdr_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Server: MiniHTTPd\r\n"
        "Connection: close\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n\r\n",
        mime, (size_t)size);

    /* Send headers */
    send(client_fd, header, hdr_len, 0);

    /* Send body in chunks */
    size_t total_sent = 0;
    while (total_sent < (size_t)size) {
        ssize_t sent = send(client_fd, body + total_sent, (size_t)size - total_sent, 0);
        if (sent <= 0) break;
        total_sent += (size_t)sent;
    }
    free(body);
}

void iniciar_servidor(const char *puerto) {
    struct sockaddr_in cltAddr;
    socklen_t szAddr = sizeof(cltAddr);
    int sd = passiveTCP(puerto, QLEN);
    if (sd < 0) {
        perror("Error in passiveTCP");
        exit(1);
    }

    int flags = fcntl(sd, F_GETFL, 0);
    fcntl(sd, F_SETFL, flags | O_NONBLOCK);

    int epfd = epoll_create1(0);
    if (epfd < 0) {
        perror("epoll_create1");
        close(sd);
        exit(1);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev) < 0) {
        perror("epoll_ctl add listen");
        close(sd);
        close(epfd);
        exit(1);
    }

    while (1) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sd) {
                int nsd = accept(sd, (struct sockaddr *)&cltAddr, &szAddr);
                if (nsd >= 0) {
                    int cflags = fcntl(nsd, F_GETFL, 0);
                    fcntl(nsd, F_SETFL, cflags | O_NONBLOCK);
                    ev.events = EPOLLIN;
                    ev.data.fd = nsd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, nsd, &ev);
                }
            } else {
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

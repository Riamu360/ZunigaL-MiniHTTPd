#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <limits.h>

#include "server.h"
#include "http.h"
#include "mime.h"
#include "files.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* Helper: send an HTTP error response and return keep_alive status */
static int send_error(int client_fd, int code, const char *msg, int keep_alive) {
    char header[512];
    size_t body_len = strlen(msg);
    int hdr_len = build_http_response_header(code, msg, "text/plain", body_len,
                                             keep_alive, header, sizeof(header));
    if (hdr_len > 0) {
        send(client_fd, header, hdr_len, 0);
        send(client_fd, msg, body_len, 0);
    }
    return keep_alive;
}

int passiveTCP(const char *service, int qlen) {
    struct addrinfo hints, *res, *p;
    int sockfd = -1;
    int opt = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;      /* bind to any address */

    int rc = getaddrinfo(NULL, service, &hints, &res);
    if (rc != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) continue;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0) break; /* success */
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

int procesar_peticion(int client_fd) {
    const size_t REQ_BUF = 8192;
    char raw[REQ_BUF];
    ssize_t n = recv(client_fd, raw, REQ_BUF - 1, 0);
    if (n <= 0) return 0; /* connection closed or error — close fd */
    raw[n] = '\0';

    /* Parse HTTP request (request line + headers) */
    http_request_t req;
    int parse_result = parse_http_request(raw, &req);

    if (parse_result == -2) {
        /* Request too large (URI, request line, or headers exceeded limits) */
        return send_error(client_fd, 400, "Bad Request", 0);
    }
    if (parse_result != 0) {
        return send_error(client_fd, 400, "Bad Request", 0);
    }

    /* Determine keep-alive based on Connection header and HTTP version */
    int keep_alive;
    if (strcasecmp(req.connection, "close") == 0) {
        keep_alive = 0;
    } else if (strcasecmp(req.connection, "keep-alive") == 0) {
        keep_alive = 1;
    } else {
        /* Default: HTTP/1.1 keeps alive, HTTP/1.0 closes */
        keep_alive = (strstr(req.version, "1.1") != NULL) ? 1 : 0;
    }

    /* Only GET is allowed */
    if (strcmp(req.method, "GET") != 0) {
        return send_error(client_fd, 405, "Method Not Allowed", keep_alive);
    }

    /* Open the requested static file */
    off_t file_size = 0;
    char real_path[PATH_MAX];
    int fd = open_static_file(req.uri, &file_size, real_path, sizeof(real_path));
    if (fd < 0) {
        int code = (fd == -403) ? 403 : 404;
        const char *msg = (code == 403) ? "Forbidden" : "Not Found";
        return send_error(client_fd, code, msg, keep_alive);
    }

    /* Determine MIME type from file extension */
    const char *mime = get_mime_type(real_path);

    /* Read file content into memory */
    char *body = malloc((size_t)file_size);
    if (!body) {
        close(fd);
        return send_error(client_fd, 500, "Internal Server Error", 0);
    }
    ssize_t read_bytes = read(fd, body, file_size);
    close(fd);
    if (read_bytes != file_size) {
        free(body);
        return send_error(client_fd, 500, "Internal Server Error", 0);
    }

    /* Build and send response header */
    char header[1024];
    int hdr_len = build_http_response_header(200, "OK", mime,
                                             (size_t)file_size, keep_alive,
                                             header, sizeof(header));
    if (hdr_len < 0) {
        free(body);
        return send_error(client_fd, 500, "Internal Server Error", 0);
    }
    send(client_fd, header, hdr_len, 0);

    /* Send body in chunks */
    size_t total_sent = 0;
    while (total_sent < (size_t)file_size) {
        ssize_t sent = send(client_fd, body + total_sent,
                            (size_t)file_size - total_sent, 0);
        if (sent <= 0) break;
        total_sent += (size_t)sent;
    }
    free(body);

    return keep_alive;
}

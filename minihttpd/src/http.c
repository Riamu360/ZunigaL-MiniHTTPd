#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include "http.h"

#define MAX_REQUEST_LINE 8192
#define MAX_LINE_BUF 1024

/* Analiza una sola línea de cabecera "Nombre: valor" y guarda las cabeceras reconocidas */
static void parse_header_line(const char *line, http_request_t *req) {
    const char *colon = strchr(line, ':');
    if (!colon) return;

    size_t name_len = (size_t)(colon - line);
    const char *value = colon + 1;
    /* Omitir espacios en blanco iniciales en el valor */
    while (*value == ' ' || *value == '\t') value++;

    if (name_len == 4 && strncasecmp(line, "Host", 4) == 0) {
        strncpy(req->host, value, sizeof(req->host) - 1);
        req->host[sizeof(req->host) - 1] = '\0';
    } else if (name_len == 10 && strncasecmp(line, "Connection", 10) == 0) {
        strncpy(req->connection, value, sizeof(req->connection) - 1);
        req->connection[sizeof(req->connection) - 1] = '\0';
    } else if (name_len == 10 && strncasecmp(line, "User-Agent", 10) == 0) {
        strncpy(req->user_agent, value, sizeof(req->user_agent) - 1);
        req->user_agent[sizeof(req->user_agent) - 1] = '\0';
    }
}

int parse_http_request(const char *raw, http_request_t *req) {
    if (!raw || !req) return -1;

    memset(req, 0, sizeof(*req));

    /* Validar el tamaño total de la petición */
    size_t raw_len = strlen(raw);
    if (raw_len > MAX_REQUEST_LINE) return -2;

    /* Extraer la primera línea (hasta \r\n) */
    const char *eol = strstr(raw, "\r\n");
    size_t line_len = eol ? (size_t)(eol - raw) : raw_len;

    if (line_len >= MAX_LINE_BUF) return -2; /* Línea de petición demasiado larga */

    char line[MAX_LINE_BUF];
    memcpy(line, raw, line_len);
    line[line_len] = '\0';

    /* Dividir la línea de petición por espacios */
    char *saveptr;
    char *tok_method  = strtok_r(line, " ", &saveptr);
    char *tok_uri     = strtok_r(NULL, " ", &saveptr);
    char *tok_version = strtok_r(NULL, " ", &saveptr);

    if (!tok_method || !tok_uri || !tok_version) return -1;

    /* Validar tamaños de campos antes de copiar */
    if (strlen(tok_method) >= sizeof(req->method))   return -2;
    if (strlen(tok_uri) >= sizeof(req->uri))         return -2;
    if (strlen(tok_version) >= sizeof(req->version)) return -2;

    strncpy(req->method, tok_method, sizeof(req->method) - 1);
    req->method[sizeof(req->method) - 1] = '\0';

    strncpy(req->uri, tok_uri, sizeof(req->uri) - 1);
    req->uri[sizeof(req->uri) - 1] = '\0';

    strncpy(req->version, tok_version, sizeof(req->version) - 1);
    req->version[sizeof(req->version) - 1] = '\0';

    /* Analizar cabeceras (Host, Connection, User-Agent) */
    if (eol) {
        const char *hdr_start = eol + 2; /* saltar después de \r\n */
        while (*hdr_start && *hdr_start != '\r') {
            const char *hdr_end = strstr(hdr_start, "\r\n");
            if (!hdr_end) break;

            size_t hdr_len = (size_t)(hdr_end - hdr_start);
            if (hdr_len == 0) break; /* línea vacía = fin de cabeceras */
            if (hdr_len >= MAX_LINE_BUF) {
                hdr_start = hdr_end + 2;
                continue; /* omitir línea de cabecera demasiado grande */
            }

            char hdr_line[MAX_LINE_BUF];
            memcpy(hdr_line, hdr_start, hdr_len);
            hdr_line[hdr_len] = '\0';

            parse_header_line(hdr_line, req);
            hdr_start = hdr_end + 2;
        }
    }

    return 0;
}

int build_http_response_header(int status_code, const char *status_msg,
                               const char *content_type, size_t body_len,
                               int keep_alive,
                               char *out, size_t out_len) {
    if (!out) return -1;

    int n = snprintf(out, out_len,
                     "HTTP/1.1 %d %s\r\n"
                     "Server: MiniHTTPd\r\n"
                     "Connection: %s\r\n",
                     status_code, status_msg ? status_msg : "",
                     keep_alive ? "keep-alive" : "close");
    if (n < 0 || (size_t)n >= out_len) return -1;
    size_t used = (size_t)n;

    if (content_type) {
        n = snprintf(out + used, out_len - used, "Content-Type: %s\r\n", content_type);
        if (n < 0 || (size_t)n >= out_len - used) return -1;
        used += (size_t)n;
    }

    n = snprintf(out + used, out_len - used, "Content-Length: %zu\r\n\r\n", body_len);
    if (n < 0 || (size_t)n >= out_len - used) return -1;
    used += (size_t)n;

    return (int)used;
}

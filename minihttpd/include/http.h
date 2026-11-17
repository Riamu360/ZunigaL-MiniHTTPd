#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <sys/types.h>

#define MAX_HEADER_VALUE 256

/* Petición HTTP analizada (línea de petición + cabeceras básicas) */
typedef struct {
    char method[16];
    char uri[256];
    char version[16];
    char host[MAX_HEADER_VALUE];
    char connection[MAX_HEADER_VALUE];
    char user_agent[MAX_HEADER_VALUE];
} http_request_t;

/* Analiza una petición HTTP sin procesar almacenada en `raw`.
   Si tiene éxito, rellena la estructura http_request_t y devuelve 0.
   Devuelve -1 si hay error de análisis, -2 si la línea de petición/URI/cabeceras son demasiado grandes (400).
*/
int parse_http_request(const char *raw, http_request_t *req);

/* Construye una cadena de cabecera de respuesta HTTP en `out` (tamaño `out_len`).
   NO incluye el cuerpo — solo cabeceras terminadas en \r\n\r\n.
   `keep_alive`: 1 para Connection: keep-alive, 0 para Connection: close.
   Devuelve el número de bytes escritos, o -1 si hay desbordamiento.
*/
int build_http_response_header(int status_code, const char *status_msg,
                               const char *content_type, size_t body_len,
                               int keep_alive,
                               char *out, size_t out_len);

#endif // HTTP_H

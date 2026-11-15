#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <sys/types.h>

#define MAX_HEADER_VALUE 256

/* Parsed HTTP request (request line + basic headers) */
typedef struct {
    char method[16];
    char uri[256];
    char version[16];
    char host[MAX_HEADER_VALUE];
    char connection[MAX_HEADER_VALUE];
    char user_agent[MAX_HEADER_VALUE];
} http_request_t;

/* Parses a raw HTTP request stored in `raw`.
   On success fills the http_request_t struct and returns 0.
   Returns -1 on parse error, -2 if request line/URI/headers are too large (400).
*/
int parse_http_request(const char *raw, http_request_t *req);

/* Builds an HTTP response header string into `out` (size `out_len`).
   Does NOT include the body — only headers terminated by \r\n\r\n.
   `keep_alive`: 1 for Connection: keep-alive, 0 for Connection: close.
   Returns number of bytes written, or -1 on overflow.
*/
int build_http_response_header(int status_code, const char *status_msg,
                               const char *content_type, size_t body_len,
                               int keep_alive,
                               char *out, size_t out_len);

#endif // HTTP_H

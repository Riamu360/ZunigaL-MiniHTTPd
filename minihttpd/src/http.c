#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "http.h"

/* Simple parser for a GET request line. Assumes request starts with the request line.
   Example request line: "GET /path HTTP/1.1" */
int parse_http_request(const char *request,
                       char *method, size_t method_len,
                       char *uri, size_t uri_len,
                       char *http_version, size_t version_len) {
    if (!request || !method || !uri || !http_version) return -1;

    // extract the first line (up to \r\n)
    const char *eol = strstr(request, "\r\n");
    size_t line_len = eol ? (size_t)(eol - request) : strlen(request);
    char line[1024];
    if (line_len >= sizeof(line)) return -1;
    memcpy(line, request, line_len);
    line[line_len] = '\0';

    // split by spaces
    char *tok_method = strtok(line, " ");
    char *tok_uri = strtok(NULL, " ");
    char *tok_version = strtok(NULL, " ");

    if (!tok_method || !tok_uri || !tok_version) return -1;
    if (strlen(tok_method) >= method_len || strlen(tok_uri) >= uri_len || strlen(tok_version) >= version_len)
        return -1;

    strcpy(method, tok_method);
    strcpy(uri, tok_uri);
    strcpy(http_version, tok_version);
    return 0;
}

int build_http_response(int status_code, const char *status_msg,
                       const char *content_type,
                       const char *body, size_t body_len,
                       char *out, size_t out_len) {
    if (!out) return -1;
    int n = snprintf(out, out_len,
                     "HTTP/1.1 %d %s\r\n"
                     "Server: MiniHTTPd\r\n"
                     "Connection: close\r\n",
                     status_code, status_msg ? status_msg : "");
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
    if (body && body_len > 0) {
        if (used + body_len > out_len) return -1;
        memcpy(out + used, body, body_len);
        used += body_len;
    }
    return (int)used;
}

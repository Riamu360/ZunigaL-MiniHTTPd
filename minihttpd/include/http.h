#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include <sys/types.h>

int parse_http_request(const char *request,
                       char *method, size_t method_len,
                       char *uri, size_t uri_len,
                       char *http_version, size_t version_len);

int build_http_response(int status_code, const char *status_msg,
                       const char *content_type,
                       const char *body, size_t body_len,
                       char *out, size_t out_len);

#endif

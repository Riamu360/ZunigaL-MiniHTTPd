#ifndef FILES_H
#define FILES_H

#include <sys/types.h>

int open_static_file(const char *uri, off_t *file_size, char *resolved_path, size_t path_len);

#endif
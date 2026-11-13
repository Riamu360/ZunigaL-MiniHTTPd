#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include "files.h"

int open_static_file(const char *uri, off_t *file_size, char *resolved_path, size_t path_len) {
    char base_dir[PATH_MAX];
    char requested[PATH_MAX];
    char real_path[PATH_MAX];

    if (!getcwd(base_dir, sizeof(base_dir))) return -1;
    strcat(base_dir, "/www");

    const char *resource = uri;
    if (strlen(resource) == 0 || strcmp(resource, "/") == 0) {
        resource = "/index.html";
    }

    snprintf(requested, sizeof(requested), "%s%s", base_dir, resource);

    if (realpath(requested, real_path) == NULL) {
        return -404; // Not Found
    }
    if (strncmp(base_dir, real_path, strlen(base_dir)) != 0) {
        return -403; // Forbidden
    }

    int fd = open(real_path, O_RDONLY);
    if (fd < 0) return -404;

    struct stat st;
    if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode)) {
        close(fd);
        return -403;
    }

    *file_size = st.st_size;
    if (resolved_path && path_len > 0) {
        strncpy(resolved_path, real_path, path_len - 1);
        resolved_path[path_len - 1] = '\0';
    }
    return fd;
}

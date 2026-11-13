#include <stdio.h>
#include <stdlib.h>
#include "server.h"

int main(int argc, char *argv[]) {
    const char *port = "80";
    if (argc > 1) {
        port = argv[1];
    }
    printf("El Servidor MiniHTTPd se está ejecutando en el puerto %s...\n", port);
    iniciar_servidor(port);
    return 0;
}

# Proyecto Bimestral - MiniHTTPd

Servidor HTTP/1.1 basico implementado en C para Linux.

## Caracteristicas

- Sirve archivos estaticos (HTML, CSS, JS, imagenes)
- Procesa solicitudes GET con analisis de encabezados HTTP/1.1 (Host, Connection, User-Agent)
- Multiplexacion de clientes mediante epoll con conexiones persistentes (keep-alive)
- Codigos de respuesta: 200, 400, 403, 404, 405, 500
- Tipos MIME: text/html, text/css, application/javascript, image/png, image/jpeg
- Proteccion contra directory traversal (realpath)
- Validacion de tamano de solicitudes (URI, linea de solicitud, encabezados)
- Rechazo de metodos distintos a GET (405 Method Not Allowed)

## Compilacion

```bash
make
```

## Ejecucion

```bash
./minihttpd
```

## Estructura

```
minihttpd/
├── Makefile
├── README.md
├── include/
│   ├── http.h      # Parsing de solicitudes y generacion de respuestas
│   ├── server.h    # Socket TCP pasivo y procesamiento de peticiones
│   ├── mime.h      # Tipos de contenido MIME
│   └── files.h     # Acceso a archivos estaticos
├── src/
│   ├── main.c      # Bucle de eventos (epoll)
│   ├── server.c    # Funciones del servidor (TCP, procesamiento)
│   ├── http.c      # Parser HTTP y constructor de respuestas
│   ├── mime.c      # Resolucion de tipos MIME
│   └── files.c     # Acceso al sistema de archivos
└── www/
    ├── index.html   # Pagina principal
    ├── style.css    # Estilos
    └── image.png    # Imagen de prueba
```

## Seguridad

- **Directory Traversal**: Se utiliza `realpath()` para resolver rutas y se verifica que la ruta resuelta este dentro del directorio `www/`.
- **Buffer Overflow**: Se usa `strncpy` y `snprintf` en lugar de `strcpy`/`sprintf`. Se validan los tamanos de los campos antes de copiar.
- **Metodos invalidos**: Solo se acepta GET; cualquier otro metodo recibe 405.
- **Tamano de solicitud**: Las solicitudes que excedan los limites reciben 400 Bad Request.

#include "include/axionetd.h"
#include "include/http.h"
#include "include/router.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AxioResponse* root(AxioRequest* request) {
    AxioResponse* resp = malloc(sizeof(AxioResponse));

    if (!resp) {
        printf("Failed to initialize response.\n");
        return NULL;
    }

    resp->response = "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, world!";

    return resp; 
}

int main(void) {
    Axionet* server = initServer(8000, 5);

    if (!server) {
        printf("Failed to create server.\n");
        return 1;
    }

    addRoute(server, "/", root);

    startServer(server);
    return 0;
}
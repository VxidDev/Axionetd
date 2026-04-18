#include "../include/defaultRoutes.h"
#include "../include/http.h"

#include <stdlib.h>
#include <string.h>

AxioResponse* route404() {
    AxioResponse* resp = malloc(sizeof(AxioResponse));

    if (!resp) {
        return NULL;
    }

    resp->response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "404 Page Not Found";

    return resp; 
}

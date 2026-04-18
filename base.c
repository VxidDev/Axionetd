#include "include/axionetd.h"
#include "include/http.h"
#include "include/router.h"

#include <stdio.h>
#include <stdlib.h>

AxioResponse* root(AxioRequest* request) {
    AxioResponse* resp = initResponse("<h1>Hello, World!</h1>", 200, "text/html");
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
    free(server);
    return 0;
}
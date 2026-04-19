#include "include/axionetd.h"
#include "include/http.h"
#include "include/router.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

AxioResponse* root(AxioRequest* request) {
    AxioHeader** headers = AxioRequest_getHeaders(request);

    for (int i = 0; headers[i]; i++) {
        printf("%s: %s\n", headers[i]->key, headers[i]->value);
    }

    AxioHeader contentType[] = {
        {"Content-Type", "text/html"}
    };

    AxioResponse* resp = initResponse("<h1>Hello, World!</h1>", 200, contentType, 1);
    return resp; 
}

int main(void) {
    Axionet* server = initServer("0.0.0.0", 8000, 8192, false);

    if (!server) {
        printf("Failed to create server.\n");
        return 1;
    }

    addRoute(server, "/", root);

    startServer(server);
    free(server);
    return 0;
}
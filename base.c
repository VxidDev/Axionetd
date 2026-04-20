#include "include/axionetd.h"
#include "include/http.h"
#include "include/router.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void root(AxioRequest* request, AxioResponse* response) {
    //for (int i = 0; i < request->headerAmount; i++) {
    //    printf("%s: %s\n", request->headers[i].key, request->headers[i].value);
    //}

    AxioHeader contentType[] = {
        {"Content-Type", "text/html"}
    };

    initResponse(response, "<h1>Hello, World!</h1>", 200, contentType, 1);
}

int main(void) {
    Axionet* server = initServer("0.0.0.0", 8000, 8192, false);

    if (!server) {
        printf("Failed to create server.\n");
        return 1;
    }

    addRoute(server, "/", (char*[]){}, 0, root);

    startServer(server);
    free(server);
    return 0;
}
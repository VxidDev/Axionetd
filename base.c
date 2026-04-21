#include "include/axionetd.h"
#include "include/http.h"
// #include "include/json.h"
#include "include/router.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void root(AxioRequest* request, AxioResponse* response) {
    /*
    char resp[128];

    if (AxioRequest_parseJSON(request)) {
        const char *name = "Default";
        AxioRequest_JSONGetStr(request, "name", &name);

        int age = 0;
        AxioRequest_JSONGetInt(request, "age", &age);
       
        bool isAlive = false;
        AxioRequest_JSONGetBool(request, "isAlive", &isAlive);

        snprintf(resp, sizeof(resp), "<h1>Hello, %s! Your age: %d. You're %s!</h1>", name, age, isAlive ? "alive" : "dead");
        HTMLResponse(response, resp, 200, NULL, 0);
    
        return;
    }
    */

    HTMLResponse(response, "<h1>Hello, World!</h1>", 200, NULL, 0);
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
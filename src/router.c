#include "../include/axionetd.h"
#include "../include/router.h"

#include <stdlib.h>
#include <stdio.h>

bool addRoute(Axionet *server, char *path, AxioResponse* (*handler)(AxioRequest *)) {
    if (!server || !path || !handler) {
        return false;
    }
    
    AxioRoute* route = malloc(sizeof(AxioRoute)); // Allocate memory for route

    if (!route) { // Handle memory allocation fail
        return false; 
    }

    // Fill in route data
    snprintf(route->path, AXIO_MAX_PATH, "%s", path);

    route->handler = handler;

    if (server->routeAmount >= server->routeCapacity) {
        size_t newCap = server->routeCapacity == 0 ? 4 : server->routeCapacity * 2;

        AxioRoute** temp = realloc(server->routes, sizeof(AxioRoute*) * server->routeCapacity);

        if (!temp) {
            free(route);
            return false;
        }

        server->routes = temp;
        server->routeCapacity = newCap;
    }

    server->routes[server->routeAmount++] = route;

    return true; 
}

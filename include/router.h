#ifndef AXIONETD_ROUTER_H
#define AXIONETD_ROUTER_H

#include "axionetd.h"
#include "config.h"
#include "http.h"

typedef struct AxioRoute {
    char path[AXIO_MAX_PATH]; // Path, e.g "/"
    char *methods[AXIO_MAX_METHODS]; // Array of methods route allows or NULL as a wildcard
    int amountOfMethods;
    void (*handler)(AxioRequest *, AxioResponse *); // Handler which should return a HTTP response.
} AxioRoute;

bool addRoute(Axionet* server, char* path, char **methods, size_t amountOfMethods, void (*handler)(AxioRequest *, AxioResponse *));

#endif // AXIONETD_ROUTER_H
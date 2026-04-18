#ifndef AXIONETD_ROUTER_H
#define AXIONETD_ROUTER_H

#include "axionetd.h"
#include "config.h"
#include "http.h"

typedef struct AxioRoute {
    char path[AXIO_MAX_PATH]; // Path, e.g "/"
    AxioResponse* (*handler)(AxioRequest *); // Handler which should return a HTTP response.
} AxioRoute;

bool addRoute(Axionet* server, char* path, AxioResponse* (*handler)(AxioRequest *));

#endif // AXIONETD_ROUTER_H
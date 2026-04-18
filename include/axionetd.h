#ifndef AXIONETD_H
#define AXIONETD_H 

#include <stdbool.h>
#include <stddef.h>

typedef struct AxioRequest AxioRequest; // http.h
typedef struct AxioResponse AxioResponse; // http.h
typedef struct AxioRoute AxioRoute; // router.h

typedef struct Axionet {
    int fd; // Listening socket
    int port;
    
    unsigned long routeAmount;
    unsigned long routeCapacity;

    int backlog; // queue size for listen()
    bool isRunning;
    AxioRoute **routes; // List of Key (path), Value (handler) pairs.
} Axionet;

extern Axionet *globalServer;

Axionet* initServer(const int port, const int backlog);
void startServer(Axionet* server);

#endif // AXIONETD_H
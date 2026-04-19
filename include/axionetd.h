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
    char* host;
    
    unsigned long routeAmount;
    unsigned long routeCapacity;

    int backlog; // queue size for listen()
    bool isRunning;
    AxioRoute **routes; // List of Key (path), Value (handler) pairs.
} Axionet;

extern Axionet *globalServer;
extern bool enableLogging;

Axionet* initServer(const char* host, const int port, const int backlog, const bool logging);
void startServer(Axionet* server);

#endif // AXIONETD_H
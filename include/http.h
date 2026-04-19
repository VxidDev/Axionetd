#ifndef AXIONETD_REQUEST_H
#define AXIONETD_REQUEST_H

#include "config.h"

typedef struct AxioHeader {
    char *key, *value;
} AxioHeader;

typedef struct AxioRequest {
    char *raw; // Raw request body

    char path[AXIO_MAX_PATH]; // Requested path, e.g "/"
    char method[AXIO_MAX_METHOD]; // Method e.g "GET"

    AxioHeader** headers; // Populated after calling AxioRequest_getHeaders
} AxioRequest;

typedef struct AxioResponse {
    char *response; // HTTP String response
    unsigned long len;
} AxioResponse;

typedef struct AxioConnection {
    int fd;

    char *response;
    unsigned long total;
    unsigned long sent;
} AxioConnection;

AxioRequest* parseRequest(char *buf);
AxioResponse* initResponse(const char* body, const int status, AxioHeader* headers, int headerCount);
AxioResponse* HTMLResponse(const char* body, const int status, AxioHeader* headers, int headerAmount);
AxioHeader** AxioRequest_getHeaders(AxioRequest* request);

#endif // AXIONETD_REQUEST_H
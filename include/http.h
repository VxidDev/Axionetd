#ifndef AXIONETD_REQUEST_H
#define AXIONETD_REQUEST_H

#include "config.h"

typedef struct AxioRequest {
    char path[AXIO_MAX_PATH]; // Requested path, e.g "/"
    char method[AXIO_MAX_METHOD]; // Method e.g "GET"
} AxioRequest;

typedef struct AxioResponse {
    char *response; // HTTP String response
} AxioResponse;

AxioRequest* parseRequest(char *buf);
AxioResponse* initResponse(const char* str, const int status, const char* contentType);

#endif // AXIONETD_REQUEST_H
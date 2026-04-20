#ifndef AXIONETD_REQUEST_H
#define AXIONETD_REQUEST_H

#include "axionetd.h"
#include "config.h"
#include <stdbool.h>
#include <yyjson.h>

typedef struct AxioHeader {
    char *key, *value;
} AxioHeader;

typedef struct AxioRequest {
    char *raw; // Raw request body

    char path[AXIO_MAX_PATH]; // Requested path, e.g "/"
    char method[AXIO_MAX_METHOD]; // Method e.g "GET"

    AxioHeader headers[AXIO_MAX_HEADERS]; // Populated after calling AxioRequest_getHeaders
    int headerAmount;

    char contentType[64];
    bool contentTypeSet;

    yyjson_doc *json;
    yyjson_val *jsonRoot;
} AxioRequest;

typedef struct AxioResponse {
    char *response; // HTTP String respons
    int status;
    unsigned long len;
} AxioResponse;

typedef struct AxioConnection {
    int fd;
    AxioState state;
    
    char *response;
    unsigned long total;
    unsigned long sent;

    char *readBuffer;
} AxioConnection;

bool parseRequest(AxioRequest* request, char *buf);
void initResponse(AxioResponse* resp, const char* body, const int status, AxioHeader* headers, int headerCount);
void HTMLResponse(AxioResponse* resp, const char* body, const int status, AxioHeader* headers, int headerAmount);

#endif // AXIONETD_REQUEST_H
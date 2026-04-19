#include "../include/http.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

bool _extract_method(char* request, char* buf, int limit) {
    int i = 0;

    // copy until space or limit - 1
    while (request[i] != ' ' && request[i] != '\0' && i < limit - 1) {
        buf[i] = request[i];
        i++;
    }

    buf[i] = '\0';

    return (i > 0); // success only if something was extracted
}

bool _extract_path(char *request, char *buf, int limit) {
    char* path = strchr(request, ' ');
    if (!path) return false;

    path++;

    int i = 0;

    // copy path until next space
    while (*path != ' ' && *path != '\0' && i < limit - 1) {
        buf[i++] = *path++;
    }

    buf[i] = '\0';
    return (i > 0);
}

AxioHeader* _extract_headers(AxioRequest* request) {
    char *p = strstr(request->raw, "\r\n");
    if (!p) return NULL;

    p += 2; // skip "\r\n"

    char *end = strstr(p, "\r\n\r\n");
    if (end) *end = '\0';

    int count = 0;
    
    while (*p && count < AXIO_MAX_HEADERS) {
        char *line = p;

        // Find end of line
        while (*p && !(p[0] == '\r' && p[1] == '\n')) p++;

        if (*p) {
            *p = '\0';
            p += 2;
        }

        // Find colon
        char *colon = line;
        while (*colon && *colon != ':') colon++;

        if (*colon == ':') {
            *colon = '\0';

            char *value = colon + 1;
            if (*value == ' ') value++;

            request->headers[count].key = line;
            request->headers[count].value = value;

            count++;
        } 
    }
    
    request->headerAmount = count; 
    return request->headers;
} 

AxioRequest* parseRequest(char *buf) {
    AxioRequest* request = malloc(sizeof(AxioRequest)); // Allocate memory for request

    if (!request) { // Handle memory allocation fail
        return NULL;
    }

    if (!_extract_method(buf, request->method, AXIO_MAX_METHOD)) { // Load method (e.g "GET") to request->method or handle error
        free(request); // Free memory
        return NULL;
    }

    if (!_extract_path(buf, request->path, AXIO_MAX_PATH)) { // Load path (e.g "/") to request->path
        free(request); // Free memory
        return NULL;
    }

    // For optimization purposes user should extract headers by using
    //   AxioHeaders** AxioRequest_getHeaders(const AxioRequest* request)

    request->raw = buf;
    _extract_headers(request); // fills in request->headers

    //request->headers = NULL;
    //request->raw = buf;

    return request;
}

AxioResponse* initResponse(const char* body, int status, AxioHeader* headers, int headerCount) {
    size_t bodyLen = strlen(body);

    // compute size manually 
    size_t needed = 0;

    // status line (estimate fixed format)
    needed += 32; // "HTTP/1.1 200\r\n" + safety margin

    needed += 22; // "Content-Length: %zu\r\n"
    needed += 20; // "Connection: close\r\n"

    for (int i = 0; i < headerCount; i++) {
        needed += strlen(headers[i].key) + strlen(headers[i].value) + 4; // ": \r\n"
    }

    needed += 2; // "\r\n"
    needed += bodyLen;
    needed += 1; // null terminator

    char *response = malloc(needed);
    if (!response) return NULL;

    char *p = response;

    // write response 
    p += sprintf(p,
        "HTTP/1.1 %d\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n",
        status, bodyLen
    );

    for (int i = 0; i < headerCount; i++) {
        p += sprintf(p, "%s: %s\r\n",
            headers[i].key,
            headers[i].value
        );
    }

    p += sprintf(p, "\r\n");

    if (bodyLen > 0) {
        memcpy(p, body, bodyLen);
        p += bodyLen;
    }

    *p = '\0';

    AxioResponse* resp = malloc(sizeof(AxioResponse));

    if (!resp) {
        free(response);
        return NULL;
    }

    resp->response = response;
    resp->len = (size_t)(p - response);

    return resp;
}

AxioResponse* HTMLResponse(const char* body, const int status, AxioHeader* headers, int headerAmount) {
    AxioHeader h[headerAmount + 1];

    for (int i = 0; i < headerAmount; i++) {
        h[i] = headers[i];
    }

    h[headerAmount] = (AxioHeader){"Content-Type", "text/html"};

    return initResponse(body, status, h, headerAmount);
}
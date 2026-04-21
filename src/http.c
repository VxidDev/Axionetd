#include "../include/http.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

bool _extract_method(AxioRequest *request) {
    int i = 0;

    // copy until space or limit - 1
    while (request->raw[i] != ' ' && request->raw[i] != '\0' && i < AXIO_MAX_METHOD - 1) {
        request->method[i] = request->raw[i];
        i++;
    }

    request->method[i] = '\0';

    return (i > 0); // success only if something was extracted
}

bool _extract_path(AxioRequest *request) {
    char* path = strchr(request->raw, ' ');
    if (!path) return false;

    path++;

    int i = 0;

    // copy path until next space
    while (*path != ' ' && *path != '\0' && i < AXIO_MAX_PATH - 1) {
        request->path[i++] = *path++;
    }

    request->path[i] = '\0';
    return (i > 0);
}

bool _extract_headers(AxioRequest* request) {
    char *p = strstr(request->raw, "\r\n");
    if (!p) return false;

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

            if (!request->contentTypeSet && strcmp(request->headers[count].key, "Content-Type") == 0) {
                int i = 0;

                for (; *line && i < 64; i++) {
                    request->contentType[i] = value[i];
                } 

                request->contentTypeSet = true;
                request->contentType[i] = '\0';
            }

            count++;
        } 
    }
    
    request->headerAmount = count;
    return true;
} 

bool AxioRequest_parseJSON(AxioRequest* request) {
    if (strcmp(request->contentType, "application/json") != 0) {
        return false;
    }

    char *body = request->headers[request->headerAmount - 1].value + strlen(request->headers[request->headerAmount - 1].value); // Beginning of body
    if (!body) return false;

    body += 4;

    request->json = yyjson_read(body, strlen(body), 0);

    return request->json != NULL;
}

bool parseRequest(AxioRequest *request, char *buf) {
    if (!request) {
        return false;
    }

    request->contentTypeSet = false;
    request->raw = buf;

    if (!_extract_method(request)) { // Load method (e.g "GET") to request->method or handle error
        return false;
    }

    if (!_extract_path(request)) { // Load path (e.g "/") to request->path
        return false;
    }

    if (!_extract_headers(request)) { // fills in request->headers
        return false;
    } 

    request->json = NULL; // Initialize
    request->jsonRoot = NULL; // Initialize

    return true;
}

void initResponse(AxioResponse *resp, const char* body, int status, AxioHeader* headers, int headerCount) {
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

    if (!response) {
        resp->response = NULL;
        resp->status = 500;
        resp->len = 0;
    } else {
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


        resp->response = response;
        resp->len = (size_t)(p - response);
        resp->status = status;
    }
}

void HTMLResponse(AxioResponse* resp, const char* body, const int status, AxioHeader* headers, int headerAmount) {
    AxioHeader h[headerAmount + 1];

    for (int i = 0; i < headerAmount; i++) {
        h[i] = headers[i];
    }

    h[headerAmount] = (AxioHeader){"Content-Type", "text/html"};

    initResponse(resp, body, status, h, headerAmount);
}
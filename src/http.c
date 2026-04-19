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

AxioHeader** _extract_headers(char* request) {
    char *headersStart = strstr(request, "\r\n");
    if (!headersStart) return NULL;

    headersStart += 2; // skip "\r\n"

    AxioHeader** headers = malloc(sizeof(AxioHeader*) * AXIO_MAX_HEADERS);
    int count = 0;

    char *line = headersStart;

    while (line && line[0] != '\0') {
        char *next = strstr(line, "\r\n");
        if (!next) break;

        *next = '\0';

        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';

            AxioHeader* header = malloc(sizeof(AxioHeader));

            header->key = line;
            header->value = colon + 1;

            headers[count++] = header;
        }

        line = next + 2;
    }

    if (count < AXIO_MAX_HEADERS) {
        headers[count] = NULL; // null-terminate 
    }

    return headers;
} 

AxioHeader** AxioRequest_getHeaders(AxioRequest* request) {
    if (!request->headers) {
        request->headers = _extract_headers(request->raw);
    }

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

    /* For optimization purposes user should extract headers by using
       AxioHeaders** AxioRequest_getHeaders(const AxioRequest* request)

    request->headers = _extract_headers(buf);

    if (!request->headers) {
        free(request);
        return NULL;
    }

    */

    request->headers = NULL;
    request->raw = buf;

    return request;
}

AxioResponse* initResponse(const char* body, const int status, AxioHeader* headers, int headerCount) {
    size_t bodyLen = strlen(body);

    // Compute size needed
    int needed = snprintf(NULL, 0,
        "HTTP/1.1 %d\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n",
        status, bodyLen
    );

    // Add custom headers
    for (int i = 0; i < headerCount; i++) {
        needed += snprintf(NULL, 0, "%s: %s\r\n", headers[i].key, headers[i].value);
    }

    // Final CRLF, body and a null-terminator
    needed += snprintf(NULL, 0, "\r\n%s", body);

    char *response = malloc(needed + 1);
    if (!response) return NULL;

    int offset = 0;

    // Write status line
    offset += snprintf(response + offset, needed + 1 - offset,
        "HTTP/1.1 %d\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n",
        status, bodyLen
    );

    // Write headers
    for (int i = 0; i < headerCount; i++) {
        offset += snprintf(response + offset, needed + 1 - offset,
            "%s: %s\r\n",
            headers[i].key,
            headers[i].value
        );
    }

    // End headers & body
    offset += snprintf(response + offset, needed + 1 - offset,
        "\r\n%s", body 
    );

    AxioResponse* resp = malloc(sizeof(AxioResponse));

    if (!resp) {
        free(response);
        return NULL;
    }

    resp->response = response;
    resp->len = offset;
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
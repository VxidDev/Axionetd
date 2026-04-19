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

    request->headers = _extract_headers(buf);

    if (!request->headers) {
        free(request);
        return NULL;
    }

    return request;
}

AxioResponse* initResponse(const char* str, const int status, const char* contentType) {
    int needed = snprintf(NULL, 0,
        "HTTP/1.1 %d\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, contentType, strlen(str), str
    );

    char *response = malloc(needed + 1);
    if (!response) return NULL;

    snprintf(response, needed + 1,
        "HTTP/1.1 %d\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status, contentType, strlen(str), str
    );

    AxioResponse* resp = malloc(sizeof(AxioResponse));

    if (!resp) {
        free(response);
        return NULL;
    }

    resp->response = response;
    resp->len = needed;
    return resp;
}

AxioResponse* HTMLResponse(const char* str, const int status) {
    return initResponse(str, status, "text/html");
}
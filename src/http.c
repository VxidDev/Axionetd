#include "../include/http.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

bool _extract_method(const char* request, char* buf, int limit) {
    int i = 0;

    // copy until space or limit - 1
    while (request[i] != ' ' && request[i] != '\0' && i < limit - 1) {
        buf[i] = request[i];
        i++;
    }

    buf[i] = '\0';

    return (i > 0); // success only if something was extracted
}

bool _extract_path(const char *request, char *buf, int limit) {
    int i = 0, j = 0;

    // skip method
    while (request[i] != ' ' && request[i] != '\0') {
        i++;
    }

    if (request[i] == ' ') {
        i++;
    }

    // copy path until next space
    while (request[i] != ' ' && request[i] != '\0' && j < limit - 1) {
        buf[j++] = request[i++];
    }

    buf[j] = '\0';
    return j > 0;
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

    return request;
}

AxioResponse* initResponse(const char* str, const int status, const char* contentType) {
    char *response = malloc(1024);

    if (!response) {
        return NULL;
    }

    snprintf(response, 1024, 
        "HTTP/1.1 %d\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s", status, contentType, strlen(str), str
    );

    AxioResponse* resp = malloc(sizeof(AxioResponse));

    if (!resp) {
        free(response);
        return NULL;
    }

    resp->response = response;
    return resp;
}
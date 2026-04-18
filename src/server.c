#include "../include/axionetd.h"
#include "../include/http.h"
#include "../include/router.h"
#include "../include/defaultRoutes.h"

#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#include <stdio.h>

Axionet* globalServer = NULL;

Axionet* initServer(const int port, const int backlog) {
    int serverFd = socket(AF_INET, SOCK_STREAM, 0); // Initialize socket

    if (serverFd == -1) { // Error while initializing socket
        return NULL;
    }

    struct sockaddr_in addr = {0}; // Fill in address structure

    addr.sin_family = AF_INET; // IPv4
    addr.sin_port = htons(port); // Port
    addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0 for now.

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) { // Bind address structure to socket and handle error
        close(serverFd); // Close socket
        return NULL;
    }

    Axionet* server = malloc(sizeof(Axionet)); // Allocate memory for server

    if (!server) { // Handle memory allocation failure
        close(serverFd); // Close socket
        return NULL;
    }

    // Fill in server structure
    server->fd = serverFd;
    server->port = port;
    server->backlog = backlog;
    server->isRunning = false;

    server->routeAmount = 0;
    server->routeCapacity = 4;

    server->routes = malloc(sizeof(AxioRoute*) * server->routeCapacity);

    if (!server->routes) {
        close(serverFd);
        free(server);
        return NULL;
    }

    return server;
}

void stopServer(int sig) {
    globalServer->isRunning = false;
}

void startServer(Axionet* server) {
    globalServer = server;

    struct sigaction sa = {0};

    sa.sa_handler = stopServer;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    if (listen(server->fd, server->backlog) == -1) { // Prepare accepting connect and handle error
        return;
    }

    server->isRunning = true;
    
    while (server->isRunning) { // Event loop
        int clientFd = accept(server->fd, NULL, NULL); // Wait for connection

        if (clientFd == -1) {
            if (!server->isRunning) break;
            continue;
        }

        char buffer[1024]; // Buffer for request reading

        ssize_t n = read(clientFd, buffer, sizeof(buffer) - 1); // Read request into buffer 

        if (n <= 0) {
            close(clientFd);
            continue;
        }

        buffer[n] = '\0';
        
        AxioRequest* request = parseRequest(buffer);

        printf("Method - %s | Route - %s\n", request->method , request->path);

        AxioResponse* response = NULL;

        for (size_t i = 0; i < server->routeAmount; i++) {
            if (strcmp(request->path, server->routes[i]->path) == 0) { // Look for matching path
                response = server->routes[i]->handler(request); // Execute path and save response
                break;
            }
        }

        if (!response || !response->response) {
            if (response) free(response);
            response = route404();
        }

        write(clientFd, response->response, strlen(response->response)); // Write response
        close(clientFd); // End request handling 

        free(request);

        free(response->response);
        free(response);
    }

    for (size_t i = 0; i < server->routeAmount; i++) {
        free(server->routes[i]);
    }

    free(server->routes);
    close(server->fd);
}
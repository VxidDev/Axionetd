#include "../include/axionetd.h"
#include "../include/http.h"
#include "../include/router.h"
#include "../include/defaultRoutes.h"

#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>

#include <stdio.h>

Axionet* globalServer = NULL;
bool enableLogging = true;

Axionet* initServer(const int port, const int backlog, const bool logging) {
    enableLogging = logging;

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
    (void)sig;

    if (globalServer) {
        globalServer->isRunning = false;
    }
}

void startServer(Axionet* server) {
    globalServer = server;

    struct sigaction sa = {0};

    sa.sa_handler = stopServer;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);

    int epollFd = epoll_create1(0);

    if (epollFd == -1) return;

    fcntl(server->fd, F_SETFL, O_NONBLOCK);

    if (listen(server->fd, server->backlog) == -1) { // Prepare accepting connect and handle error
        return;
    }

    server->isRunning = true;

    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.fd = server->fd;

    epoll_ctl(epollFd, EPOLL_CTL_ADD, server->fd, &ev);

    struct epoll_event events[64];
        
    while (server->isRunning) { // Event loop
        int nready = epoll_wait(epollFd, events, 64, -1); // Wait for connection

        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;

            if (fd == server->fd) { // New connection
                int clientFd = accept(server->fd, NULL, NULL);

                if (clientFd == -1) {
                    if (!server->isRunning) break;
                    continue;
                }

                fcntl(clientFd, F_SETFL, O_NONBLOCK);
                
                struct epoll_event cev = {0};
                cev.events = EPOLLIN;
                cev.data.fd = clientFd;

                epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &cev);
            } else { // Client request ready
                char buffer[1024]; // Buffer for request reading

                ssize_t n = read(fd, buffer, sizeof(buffer) - 1); // Read request into buffer 

                if (n <= 0) {
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }

                buffer[n] = '\0';
                
                AxioRequest* request = parseRequest(buffer);

                if (enableLogging && request) {
                    printf("Method - %s | Route - %s\n", request->method , request->path);
                }

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

                // Write response
                size_t total = strlen(response->response);
                size_t sent = 0;

                while (sent < total) {
                    ssize_t w = write(fd, response->response + sent, total - sent);
                    if (w <= 0) break;
                    sent += w;
                }
                
                // End request handling
                epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL); 
                close(fd);

                free(request);
                free(response->response);
                free(response);
            }
        }
    }

    for (size_t i = 0; i < server->routeAmount; i++) {
        free(server->routes[i]);
    }

    free(server->routes);
    close(server->fd);
    close(epollFd);
}
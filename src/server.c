#include "../include/axionetd.h"
#include "../include/http.h"
#include "../include/router.h"
#include "../include/defaultRoutes.h"

#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netdb.h>
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

void closeConnection(int epollFd, AxioConnection *conn) {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, conn->fd, NULL);
    close(conn->fd);

    if (conn->response) free(conn->response);
    free(conn);
}

Axionet* initServer(const char *host, const int port, const int backlog, const bool logging) {
    enableLogging = logging;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0); // Initialize socket

    if (serverFd == -1) { // Error while initializing socket
        return NULL;
    }

    struct addrinfo hints, *res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char portStr[16];
    snprintf(portStr, sizeof(portStr), "%d", port);

    const char *bindHost = host;

    if (!bindHost || strlen(bindHost) == 0) {
        bindHost = NULL;
    }

    if (getaddrinfo(bindHost, portStr, &hints, &res) != 0 || !res) {
        close(serverFd);
        return NULL;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(serverFd, res->ai_addr, res->ai_addrlen) == -1) { // Bind address structure to socket and handle error
        freeaddrinfo(res);
        close(serverFd); // Close socket
        return NULL;
    }

    freeaddrinfo(res);

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

    // Handle CTRL + C
    struct sigaction sa = {0};

    sa.sa_handler = stopServer;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    //

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
            // Server Socket
            if (events[i].data.fd == server->fd) { // New connection
                while (true) {
                    int clientFd = accept(server->fd, NULL, NULL);

                    if (clientFd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        else
                            continue;
                    }

                    fcntl(clientFd, F_SETFL, O_NONBLOCK);
                    
                    AxioConnection *conn = malloc(sizeof(AxioConnection));

                    conn->fd = clientFd;
                    conn->response = NULL;
                    conn->total = 0;
                    conn->sent = 0;

                    struct epoll_event cev = {0};
                    cev.events = EPOLLIN;
                    cev.data.ptr = conn;

                    epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &cev);
                }
            } else { // Client request ready
                AxioConnection *conn = events[i].data.ptr;

                // Error / hangup
                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    closeConnection(epollFd, conn);
                    continue;
                }

                // Read
                if (events[i].events & EPOLLIN) {
                    char buffer[1024];

                    ssize_t n = read(conn->fd, buffer, sizeof(buffer) - 1);

                    if (n <= 0) {
                        closeConnection(epollFd, conn);
                        continue;
                    }

                    buffer[n] = '\0';

                    AxioRequest* request = parseRequest(buffer);

                    if (enableLogging && request) {
                        printf("Method: %s | Path: %s\n", request->method, request->path);
                    }

                    AxioResponse* response = NULL;

                    for (size_t j = 0; j < server->routeAmount; j++) {
                        if (strcmp(request->path, server->routes[j]->path) == 0) {
                            response = server->routes[j]->handler(request);
                            break;
                        }
                    }

                    if (!response || !response->response) {
                        if (response) free(response);
                        response = route404();
                    }

                    // Store response
                    conn->response = response->response;
                    conn->total = strlen(conn->response);
                    conn->sent = 0;

                    // Switch to write mode
                    struct epoll_event wev = {0};
                    wev.events = EPOLLOUT;
                    wev.data.ptr = conn;

                    epoll_ctl(epollFd, EPOLL_CTL_MOD, conn->fd, &wev);

                    // Cleanup request
                    if (request->headers) {
                        for (int k = 0; request->headers[k]; k++) {
                            free(request->headers[k]);
                        }

                        free(request->headers);
                    }
                
                    free(request);
                    free(response); // Keep response->response
                }
                
                // Write
                if (events[i].events & EPOLLOUT) {
                    while (conn->sent < conn->total) {
                        ssize_t w = write(conn->fd, conn->response + conn->sent, conn->total - conn->sent);

                        if (w > 0) {
                            conn->sent += w;
                        } else if (w == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                            break; // Wait for next EPOLLOUT
                        } else {
                            closeConnection(epollFd, conn);
                            goto nextEvent;
                        }
                    }

                    if (conn->sent == conn->total) {
                        closeConnection(epollFd, conn);
                    }
                }
                
                nextEvent:;

                }
            }
        }

        // Cleanup
        for (size_t i = 0; i < server->routeAmount; i++) {
            free(server->routes[i]);
        }

        free(server->routes);
        close(server->fd);
        close(epollFd);
}
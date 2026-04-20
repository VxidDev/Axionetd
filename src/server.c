#define _GNU_SOURCE

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

const char* statusCodeToText(int status) {
    switch (status / 100) {
        case 1: return "\033[37mInformational";
        case 2: return "\033[32mSuccess";
        case 3: return "\033[33mRedirection";
        case 4: return "\033[31mClient Error";
        case 5: return "\033[35mServer Error";
        default: return "\033[90mUnknown";
    }
}

bool isMethodAllowed(const AxioRoute *route, const char *method) {
    if (!method || !route) return false;

    // Empty array = allow all
    if (!route->methods[0] || route->amountOfMethods == 0) return true;

    for (size_t i = 0; i < route->amountOfMethods; i++) {
        if (route->methods[i] && strcmp(route->methods[i], method) == 0) return true; 
    }

    return false;
};

void closeConnection(int epollFd, AxioConnection *conn) {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, conn->fd, NULL);
    close(conn->fd);

    if (conn->response) free(conn->response);
    if (conn->readBuffer) free(conn->readBuffer);
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
    server->host = strdup(bindHost ? bindHost : "0.0.0.0");
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
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server->fd;

    epoll_ctl(epollFd, EPOLL_CTL_ADD, server->fd, &ev);

    struct epoll_event events[64];

    printf("\033[1;35mAxionetd: \033[33mServer is running on \033[36mhttp://%s:%d\033[0m\n", server->host, server->port);
        
    while (server->isRunning) { // Event loop
        int nready = epoll_wait(epollFd, events, 64, -1); // Wait for connection
        if (nready <= 0) continue;

        for (int i = 0; i < nready; i++) {
            // Server Socket
            if (events[i].data.fd == server->fd) { // New connection
                while (true) {
                    int clientFd = accept4(server->fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);

                    if (clientFd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        else
                            continue;
                    }
                    
                    AxioConnection *conn = malloc(sizeof(AxioConnection));

                    conn->fd = clientFd;
                    conn->response = NULL;
                    conn->total = 0;
                    conn->sent = 0;
                    conn->state = AXIO_READING;

                    struct epoll_event cev = {0};
                    cev.events = EPOLLIN | EPOLLET ;
                    cev.data.ptr = conn;

                    epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &cev);
                }

                continue;
            } 
            // Client request ready
            AxioConnection *conn = events[i].data.ptr;

            // Error / hangup
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                closeConnection(epollFd, conn);
                continue;
            }

            // Read
            if (conn->state == AXIO_READING && (events[i].events & EPOLLIN)) {
                size_t bufSize = 4096;
                size_t total = 0;

                char *buffer = malloc(bufSize);

                while (1) {
                    // grow if needed
                    if (total >= bufSize - 1) {
                        bufSize *= 2;
                        char *tmp = realloc(buffer, bufSize);

                        if (!tmp) {
                            free(buffer);
                            closeConnection(epollFd, conn);
                            goto nextEvent;
                        }

                        buffer = tmp;
                    }

                    ssize_t n = read(conn->fd, buffer + total, bufSize - total - 1);

                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }

                        closeConnection(epollFd, conn);
                        goto nextEvent;
                    }

                    if (n == 0) break; // connection closed

                    total += n;

                    // stop early if full HTTP request received
                    if (strstr(buffer, "\r\n\r\n")) {
                        break;
                    }
                }

                buffer[total] = '\0';

                conn->readBuffer = buffer;

                AxioRequest request = {0};
                AxioResponse response = {0};

                if (!parseRequest(&request, buffer)) { // Fill in request or handle error 
                    if (enableLogging) {
                        printf("\033[1;31mERROR:\033[33m Malformed request. \033[0m\n");
                    }

                    route400(&response);
                } else {
                    for (size_t j = 0; j < server->routeAmount; j++) {
                        if (strcmp(request.path, server->routes[j]->path) == 0) {
                            if (!isMethodAllowed(server->routes[j], request.method)) {
                                route405(&response);
                                break;
                            }

                            server->routes[j]->handler(&request, &response);
                            break;
                        }
                    }

                    if (!response.response) {
                        route404(&response);
                    }
                }

                if (enableLogging) {
                    printf("\033[1;35m%s\033[37m - \033[36m%s %d %s\033[0m\n",
                        request.method, request.path, response.status, statusCodeToText(response.status)
                    );
                }

                // Store response
                conn->response = response.response;
                response.response = NULL;

                conn->total = strlen(conn->response);
                conn->sent = 0;
                conn->state = AXIO_WRITING;

                struct epoll_event wev = {0};
                wev.events    = EPOLLOUT | EPOLLET;
                wev.data.ptr  = conn;
                epoll_ctl(epollFd, EPOLL_CTL_MOD, conn->fd, &wev);

                yyjson_doc_free(request.json);
                
                continue;
            }

            if (conn->state == AXIO_WRITING) {
                while (conn->sent < conn->total) {
                    ssize_t w = write(conn->fd, conn->response + conn->sent, conn->total - conn->sent);

                    if (w > 0) {
                        conn->sent += w;
                    } else if (w == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        break;
                    } else {
                        closeConnection(epollFd, conn);
                        goto nextEvent;
                    }
                }

                if (conn->sent == conn->total) {
                    closeConnection(epollFd, conn);
                }
            }
        }
            
        nextEvent:;
    }

    // Cleanup
    for (size_t i = 0; i < server->routeAmount; i++) {
        free(server->routes[i]);
    }

    free(server->routes);
    free(server->host);
    close(server->fd);
    close(epollFd);
}
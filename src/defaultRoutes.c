#include "../include/defaultRoutes.h"
#include "../include/http.h"

AxioResponse* route404() {
    AxioResponse* resp = initResponse("<h1>404 Not Found</h1>", 404, "text/html");
    return resp; 
}

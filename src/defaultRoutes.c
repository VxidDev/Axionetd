#include "../include/defaultRoutes.h"
#include "../include/http.h"

AxioResponse* route404() {
    AxioResponse* resp = HTMLResponse("<h1>404 Not Found</h1>", 404, NULL, 0);
    return resp; 
}

AxioResponse* route405() {
    AxioResponse* resp = HTMLResponse("<h1>405 Method Not Allowed</h1>", 405, NULL, 0);
    return resp;
}

# AxionetD

AxionetD is a lightweight HTTP server framework written in C. It provides a minimal and efficient way to create HTTP servers with custom routing and request handling using raw sockets.

## Features

- Raw TCP-based HTTP server
- Simple routing system (`path -> handler`)
- Custom request parsing (method and path)
- Dynamic response handling
- SIGINT (Ctrl+C) graceful shutdown support
- Minimal dependencies (POSIX sockets + standard C library)

## Project Structure

```

include/     - Header files
src/         - Source files

````

## Example Usage

```c
Axionet* server = initServer(8000, 10);

addRoute(server, "/", rootHandler);
addRoute(server, "/hello", helloHandler);

startServer(server);
free(server);
````

## Route Handler Example

```c
AxioResponse* root(AxioRequest* request) {
    AxioResponse* resp = HTMLResponse("<h1>Hello, World!</h1>", 200);
    return resp; 
}
```

## Building

Build using `make`:
```bash
make
```

this produces a `libaxionetd.a` file.

## Notes

* Routes are matched using exact string comparison
* Responses are manually managed (must be freed)
* Only basic HTTP request parsing is currently implemented

## License

GPL-3.0 license 

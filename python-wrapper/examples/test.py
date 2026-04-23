from axionet import init_response, init_server, start_server, add_route, cstr_to_py

def my_handler(req, resp):
    path = cstr_to_py(req.contents.path)
    method = cstr_to_py(req.contents.method)

    print(f"[Python] {method} {path}")

    init_response(
        resp,
        "<h1>Hello from Python!</h1>",
        200,
        [("Content-Type", "text/html")]
    )

try:
    server = init_server("127.0.0.1", 8080, 10, True)

    if add_route(server, "/hello", ["GET"], my_handler):
        print("Route added.")
    else:
        print("Failed to add route.")

    start_server(server)

except RuntimeError as e:
    print(f"Error: {e}")

except KeyboardInterrupt:
    print("Server stopped.")
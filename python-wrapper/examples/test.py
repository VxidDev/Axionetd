from axionet import cstr_to_py, AxionetInstance

def my_handler(req, resp):
    path = cstr_to_py(req.contents.path)
    method = cstr_to_py(req.contents.method)

    print(f"[Python] {method} {path}")

    server.init_response(
        resp,
        "<h1>Hello from Python!</h1>",
        200,
        [("Content-Type", "text/html")]
    )

try:
    server = AxionetInstance("127.0.0.1", 8080, 10, 4, True)
    server.init_server()

    if server.add_route("/hello", ["GET"], my_handler):
        print("Route added.")
    else:
        print("Failed to add route.")

    server.start_server()

except RuntimeError as e:
    print(f"Error: {e}")

except KeyboardInterrupt:
    print("Server stopped.")
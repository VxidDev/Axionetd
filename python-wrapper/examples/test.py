from axionet import cstr_to_py, AxionetInstance

server = AxionetInstance("127.0.0.1", 8080, 10, 4, True)
server.init_server()

@server.route("/", ["GET"])
def root(req, resp, memory_pool):
    path = cstr_to_py(req.contents.path)
    method = cstr_to_py(req.contents.method)

    print(f"[Python] {method} {path}")

    return "<h1>Hello from Python!</h1>", 250, [("Content-Type", "text/html")]

if __name__ == "__main__":
    server.start_server()
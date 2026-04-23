import ctypes

from .lib import _lib

from .definitions.http import HANDLER_CALLBACK, AxioRoute, AxioHeader, AxioResponse, AxioRequest
from .definitions.server import Axionet
from .definitions.constants import AXIO_MAX_HEADERS, AXIO_MAX_METHODS, AXIO_MAX_METHOD, AXIO_MAX_PATH

# Prevent GC
_py_callbacks = []

# Function signatures
_lib.initServer.argtypes = [
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.c_bool
]

_lib.initServer.restype = ctypes.POINTER(Axionet)

_lib.startServer.argtypes = [
    ctypes.POINTER(Axionet)
]

_lib.startServer.restype = None

_lib.addRoute.argtypes = [
    ctypes.POINTER(Axionet),
    ctypes.c_char_p,
    ctypes.POINTER(ctypes.c_char_p),
    ctypes.c_int,
    HANDLER_CALLBACK,
    ctypes.POINTER(AxioRoute),
    ctypes.c_bool
]
_lib.addRoute.restype = ctypes.c_bool

_lib.initResponse.argtypes = [
    ctypes.POINTER(AxioResponse),
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.POINTER(AxioHeader),
    ctypes.c_int
]
_lib.initResponse.restype = None

# Wrapper functions
def init_server(host: str, port: int, backlog: int, logging: bool):
    host_bytes = host.encode('utf-8')
    server_ptr = _lib.initServer(host_bytes, port, backlog, logging)

    if not server_ptr:
        raise RuntimeError("Failed to initialize server.")

    return server_ptr

def start_server(server_ptr):
    _lib.startServer(server_ptr)

def add_route(server_ptr, path: str, methods: list[str], handler_func, threaded: bool = False):
    c_path = path.encode('utf-8')

    # Build methods array
    c_methods_array = (ctypes.c_char_p * len(methods))()

    for i, method in enumerate(methods):
        c_methods_array[i] = method.encode('utf-8')

    # Bridge C -> Python
    @HANDLER_CALLBACK
    def c_handler(c_request_ptr, c_response_ptr):
        try:
            handler_func(c_request_ptr, c_response_ptr)
        except Exception as e:
            print(f"[Axionet Python handler error] {e}")

    route_instance = AxioRoute()

    # Prevent GC
    _py_callbacks.append((c_handler, route_instance, c_methods_array))

    result = _lib.addRoute(
        server_ptr,
        c_path,
        c_methods_array,
        len(methods),
        c_handler,
        ctypes.byref(route_instance),
        threaded
    )

    return result

def init_response(resp_ptr, body: str, status: int, headers=None):
    c_body = body.encode('utf-8')

    c_headers_array = None
    header_count = 0

    if headers:
        header_count = min(len(headers), AXIO_MAX_HEADERS)
        c_headers_array = (AxioHeader * header_count)()

        for i in range(header_count):
            key_bytes = headers[i][0].encode('utf-8')
            value_bytes = headers[i][1].encode('utf-8')

            c_headers_array[i].key = key_bytes
            c_headers_array[i].value = value_bytes
            c_headers_array[i].klen = len(key_bytes)
            c_headers_array[i].vlen = len(value_bytes)

    _lib.initResponse(
        resp_ptr,
        c_body,
        status,
        c_headers_array if c_headers_array else None,
        header_count
    )

# --- Helper for safe string decoding ---
def cstr_to_py(c_array):
    return bytes(c_array).split(b'\x00', 1)[0].decode()
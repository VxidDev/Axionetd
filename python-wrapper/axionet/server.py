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

class AxionetInstance:
    def __init__(self, host: str, port: int, backlog: int, workers: int, logging: bool):
        self._host = host
        self._port = port
        self._backlog = backlog
        self._workers = workers
        self._logging = logging

        self._server = None

    def init_server(self):
        host_bytes = self._host.encode('utf-8')
        self._server = _lib.initServer(host_bytes, self._port, self._backlog, self._workers, self._logging)

        if not self._server:
            raise RuntimeError("Failed to initialize server.")

    def start_server(self):
        _lib.startServer(self._server)

    def route(self, path: str, methods: list[str], threaded: bool = False):
        def decorator(func: callable):
            def wrapper(req, resp):
                result = func(req, resp)

                if isinstance(result, tuple):
                    length = len(result)

                    body = result[0]
                    status_code = result[1] if length > 1 else 200
                    headers = result[2] if length > 2 else []

                    return self.init_response(resp, body, status_code, headers)

                return self.init_response(resp, result, 200, [])

            self.add_route(path, methods, wrapper, threaded)

            return wrapper

        return decorator

    def add_route(self, path: str, methods: list[str], handler_func, threaded: bool = False):
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
                print(f"[Handler Error] {str(e)}")

        route_instance = AxioRoute()

        # Prevent GC
        _py_callbacks.append((c_handler, route_instance, c_methods_array))

        result = _lib.addRoute(
            self._server,
            c_path,
            c_methods_array,
            len(methods),
            c_handler,
            ctypes.byref(route_instance),
            threaded
        )

        return result

    @staticmethod
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
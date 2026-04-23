import ctypes
from .constants import AXIO_MAX_HEADERS, AXIO_MAX_METHODS, AXIO_MAX_METHOD, AXIO_MAX_PATH

class AxioRequest(ctypes.Structure):
    pass

class AxioResponse(ctypes.Structure):
    pass

class AxioRoute(ctypes.Structure):
    pass

class AxioHeader(ctypes.Structure):
    _fields_ = [
        ("key", ctypes.c_char_p),
        ("value", ctypes.c_char_p),
        ("klen", ctypes.c_size_t),
        ("vlen", ctypes.c_size_t),
    ]

HANDLER_CALLBACK = ctypes.CFUNCTYPE(
    None,
    ctypes.POINTER(AxioRequest),
    ctypes.POINTER(AxioResponse)
)

YYJSON_DOC_PTR = ctypes.c_void_p
YYJSON_VAL_PTR = ctypes.c_void_p

AxioRequest._fields_ = [
    ("raw", ctypes.c_char_p),
    ("path", ctypes.c_char * AXIO_MAX_PATH),
    ("method", ctypes.c_char * AXIO_MAX_METHOD),
    ("headers", AxioHeader * AXIO_MAX_HEADERS),
    ("headerAmount", ctypes.c_int),
    ("contentType", ctypes.c_char * 64),
    ("contentTypeSet", ctypes.c_bool),
    ("json", YYJSON_DOC_PTR),
    ("jsonRoot", YYJSON_VAL_PTR),
]

AxioResponse._fields_ = [
    ("response", ctypes.c_char_p),
    ("status", ctypes.c_int),
    ("len", ctypes.c_ulong),
]

AxioRoute._fields_ = [
    ("path", ctypes.c_char * AXIO_MAX_PATH),
    ("methods", ctypes.c_char_p * AXIO_MAX_METHODS),
    ("amountOfMethods", ctypes.c_int),
    ("handler", HANDLER_CALLBACK),
    ("threaded", ctypes.c_bool),
]
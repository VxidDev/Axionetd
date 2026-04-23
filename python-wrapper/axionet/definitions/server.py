import ctypes

class Axionet(ctypes.Structure):
    _fields_ = [
        ("fd", ctypes.c_int),
        ("port", ctypes.c_int),
        ("host", ctypes.c_char_p),
        ("routeAmount", ctypes.c_ulong),
        ("routeCapacity", ctypes.c_ulong),
        ("backlog", ctypes.c_int),
        ("isRunning", ctypes.c_bool),
        ("routes", ctypes.c_void_p),
    ]
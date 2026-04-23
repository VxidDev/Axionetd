import ctypes
import os

# Define the path to the shared library
_LIB_PATH = os.path.join(os.path.dirname(os.path.dirname(__file__)), "libaxionetd.so")

try:
    _lib = ctypes.CDLL(_LIB_PATH)
except OSError as e:
    raise RuntimeError(f"Failed to load shared library {_LIB_PATH}: {e}")
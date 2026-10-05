import ctypes
from pathlib import Path


# Project root directory
PROJECT_ROOT = Path(__file__).resolve().parent.parent

# Load our compiled C library
library_path = PROJECT_ROOT / "libinventory.so"
lib = ctypes.CDLL(str(library_path))


class Database(ctypes.Structure):
    _fields_ = [
        ("connection", ctypes.c_void_p)
    ]


# Describe database_open() to Python
lib.database_open.argtypes = [
    ctypes.POINTER(Database),
    ctypes.c_char_p
]
lib.database_open.restype = ctypes.c_int


# Describe database_close() to Python
lib.database_close.argtypes = [
    ctypes.POINTER(Database)
]
lib.database_close.restype = None


# Describe inventory_get_quantity() to Python
lib.inventory_get_quantity.argtypes = [
    ctypes.POINTER(Database),
    ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_int)
]
lib.inventory_get_quantity.restype = ctypes.c_int


db = Database()

database_path = PROJECT_ROOT / "data" / "inventory.db"

rc = lib.database_open(
    ctypes.byref(db),
    str(database_path).encode("utf-8")
)

if rc != 0:
    print("Could not open database.")
    raise SystemExit(1)


quantity = ctypes.c_int()

rc = lib.inventory_get_quantity(
    ctypes.byref(db),
    1,                      # Coke Zero
    4,                      # Refrigerator
    ctypes.byref(quantity)
)

if rc == 0:
    print(f"Coke Zero in refrigerator: {quantity.value} cans")
else:
    print("Could not read inventory.")


lib.database_close(ctypes.byref(db))
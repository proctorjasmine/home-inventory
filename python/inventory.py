import ctypes
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent
LIBRARY_PATH = PROJECT_ROOT / "libinventory.so"


class Database(ctypes.Structure):
    _fields_ = [
        ("connection", ctypes.c_void_p)
    ]

class InventoryItem(ctypes.Structure):
    _fields_ = [
        ("product_id", ctypes.c_int),
        ("location_id", ctypes.c_int),
        ("product_name", ctypes.c_char * 128),
        ("inventory_unit", ctypes.c_char * 32),
        ("location_name", ctypes.c_char * 128),
        ("quantity", ctypes.c_int),
    ]

class Inventory:
    def __init__(self, database_path):
        self.lib = ctypes.CDLL(str(LIBRARY_PATH))

        self._configure_library()

        self.db = Database()

        rc = self.lib.database_open(
            ctypes.byref(self.db),
            str(database_path).encode("utf-8")
        )

        if rc != 0:
            raise RuntimeError("Could not open inventory database.")

        self._closed = False

    def _configure_library(self):
        self.lib.database_open.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_char_p
        ]
        self.lib.database_open.restype = ctypes.c_int

        self.lib.database_close.argtypes = [
            ctypes.POINTER(Database)
        ]
        self.lib.database_close.restype = None

        self.lib.inventory_get_quantity.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int)
        ]
        self.lib.inventory_get_quantity.restype = ctypes.c_int

        self.lib.inventory_list.argtypes = [
            ctypes.POINTER(Database),
            ctypes.POINTER(InventoryItem),
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int)
        ]
        self.lib.inventory_list.restype = ctypes.c_int

    def get_quantity(self, product_id, location_id):
        quantity = ctypes.c_int()

        rc = self.lib.inventory_get_quantity(
            ctypes.byref(self.db),
            product_id,
            location_id,
            ctypes.byref(quantity)
        )

        if rc != 0:
            raise RuntimeError("Could not read inventory quantity.")

        return quantity.value

    def close(self):
        if not self._closed:
            self.lib.database_close(
                ctypes.byref(self.db)
            )

            self._closed = True

    def list(self):
        max_items = 100

        items = (InventoryItem * max_items)()
        item_count = ctypes.c_int()

        rc = self.lib.inventory_list(
            ctypes.byref(self.db),
            items,
            max_items,
            ctypes.byref(item_count)
        )

        if rc != 0:
            raise RuntimeError("Could not read inventory.")

        result = []

        for i in range(item_count.value):
            item = items[i]

            result.append({
                "product_id": item.product_id,
                "location_id": item.location_id,
                "product": item.product_name.decode("utf-8"),
                "location": item.location_name.decode("utf-8"),
                "quantity": item.quantity,
                "unit": item.inventory_unit.decode("utf-8"),
            })

        return result
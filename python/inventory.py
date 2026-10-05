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

class ProductPackage(ctypes.Structure):
    _fields_ = [
        ("product_id", ctypes.c_int),
        ("package_id", ctypes.c_int),
        ("barcode", ctypes.c_char * 64),
        ("product_name", ctypes.c_char * 128),
        ("brand", ctypes.c_char * 128),
        ("inventory_unit", ctypes.c_char * 32),
        ("package_quantity", ctypes.c_int),
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

        self.lib.product_find_by_barcode.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_char_p,
            ctypes.POINTER(ProductPackage)
        ]
        self.lib.product_find_by_barcode.restype = ctypes.c_int

        self.lib.inventory_add.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int
        ]
        self.lib.inventory_add.restype = ctypes.c_int

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

    def find_barcode(self, barcode):
        package = ProductPackage()

        rc = self.lib.product_find_by_barcode(
            ctypes.byref(self.db),
            barcode.encode("utf-8"),
            ctypes.byref(package)
        )

        if rc == 1:
            return None

        if rc != 0:
            raise RuntimeError("Could not look up barcode.")

        return {
            "product_id": package.product_id,
            "package_id": package.package_id,
            "barcode": package.barcode.decode("utf-8"),
            "product": package.product_name.decode("utf-8"),
            "brand": package.brand.decode("utf-8"),
            "unit": package.inventory_unit.decode("utf-8"),
            "package_quantity": package.package_quantity,
        }

    def add(self, product_id, location_id, user_id, quantity):
        rc = self.lib.inventory_add(
            ctypes.byref(self.db),
            product_id,
            location_id,
            user_id,
            quantity
        )

        if rc != 0:
            raise RuntimeError("Could not add inventory.")
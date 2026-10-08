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

class InventoryTransaction(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("product_id", ctypes.c_int),
        ("location_id", ctypes.c_int),
        ("user_id", ctypes.c_int),
        ("product_name", ctypes.c_char * 128),
        ("inventory_unit", ctypes.c_char * 32),
        ("location_name", ctypes.c_char * 128),
        ("user_name", ctypes.c_char * 128),
        ("transaction_type", ctypes.c_char * 16),
        ("quantity_change", ctypes.c_int),
        ("previous_quantity", ctypes.c_int),
        ("new_quantity", ctypes.c_int),
        ("destination_location_id", ctypes.c_int),
        ("destination_location_name", ctypes.c_char * 128),
        ("created_at", ctypes.c_char * 32),
    ]

class Product(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("name", ctypes.c_char * 128),
        ("brand", ctypes.c_char * 128),
        ("inventory_unit", ctypes.c_char * 32),
        ("low_stock_threshold", ctypes.c_int),
        ("auto_add_grocery", ctypes.c_int),
        ("total_quantity", ctypes.c_int),
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

        self.lib.inventory_transaction_list.argtypes = [
            ctypes.POINTER(Database),
            ctypes.POINTER(InventoryTransaction),
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int)
        ]
        self.lib.inventory_transaction_list.restype = ctypes.c_int

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

        self.lib.product_create_with_package.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_char_p,       # name
            ctypes.c_char_p,       # brand
            ctypes.c_char_p,       # inventory_unit
            ctypes.c_int,          # low_stock_threshold
            ctypes.c_int,          # auto_add_grocery
            ctypes.c_char_p,       # barcode
            ctypes.c_int,          # package_quantity
            ctypes.POINTER(ctypes.c_int)  # product_id_out
        ]

        self.lib.product_create_with_package.restype = ctypes.c_int


        self.lib.product_add_package.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_int,       # product_id
            ctypes.c_char_p,    # barcode
            ctypes.c_int        # package_quantity
        ]

        self.lib.product_add_package.restype = ctypes.c_int

        self.lib.inventory_remove.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int
        ]
        self.lib.inventory_remove.restype = ctypes.c_int

        self.lib.inventory_adjust.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int
        ]
        self.lib.inventory_adjust.restype = ctypes.c_int

        self.lib.inventory_move.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_int
        ]
        self.lib.inventory_move.restype = ctypes.c_int

        self.lib.user_list.argtypes = [
            ctypes.POINTER(Database),
            ctypes.POINTER(User),
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int)
        ]
        self.lib.user_list.restype = ctypes.c_int


        self.lib.location_list.argtypes = [
            ctypes.POINTER(Database),
            ctypes.POINTER(Location),
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int)
        ]
        self.lib.location_list.restype = ctypes.c_int

        self.lib.product_list.argtypes = [
            ctypes.POINTER(Database),
            ctypes.POINTER(Product),
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int)
        ]
        self.lib.product_list.restype = ctypes.c_int

        self.lib.product_update.argtypes = [
            ctypes.POINTER(Database),
            ctypes.c_int,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_int
        ]
        self.lib.product_update.restype = ctypes.c_int

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

    def list_transactions(self):
        max_transactions = 100

        transactions = (
            InventoryTransaction * max_transactions
        )()

        transaction_count = ctypes.c_int()

        rc = self.lib.inventory_transaction_list(
            ctypes.byref(self.db),
            transactions,
            max_transactions,
            ctypes.byref(transaction_count)
        )

        if rc != 0:
            raise RuntimeError(
                "Could not read transaction history."
            )

        result = []

        for i in range(transaction_count.value):
            transaction = transactions[i]

            result.append({
                "id": transaction.id,
                "product_id": transaction.product_id,
                "location_id": transaction.location_id,
                "user_id": transaction.user_id,
                "product": transaction.product_name.decode("utf-8"),
                "unit": transaction.inventory_unit.decode("utf-8"),
                "location": transaction.location_name.decode("utf-8"),
                "user": transaction.user_name.decode("utf-8"),
                "transaction_type":
                    transaction.transaction_type.decode("utf-8"),
                "quantity_change": transaction.quantity_change,
                "previous_quantity": (
                    None
                    if transaction.previous_quantity < 0
                    else transaction.previous_quantity
                ),
                "new_quantity": (
                    None
                    if transaction.new_quantity < 0
                    else transaction.new_quantity
                ),
                "destination_location_id": (
                    transaction.destination_location_id
                    if transaction.destination_location_id > 0
                    else None
                ),
                "destination_location": (
                    transaction.destination_location_name.decode("utf-8")
                    or None
                ),
                "created_at": transaction.created_at.decode("utf-8"),
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

    def remove(self, product_id, location_id, user_id, quantity):
        rc = self.lib.inventory_remove(
            ctypes.byref(self.db),
            product_id,
            location_id,
            user_id,
            quantity
        )

        if rc == 1:
            return False

        if rc != 0:
            raise RuntimeError("Could not remove inventory.")

        return True

    def adjust(self, product_id, location_id, user_id, new_quantity):
        rc = self.lib.inventory_adjust(
            ctypes.byref(self.db),
            product_id,
            location_id,
            user_id,
            new_quantity
        )

        if rc == 1:
            return False

        if rc != 0:
            raise RuntimeError("Could not adjust inventory.")

        return True

    def move(
        self,
        product_id,
        source_location_id,
        destination_location_id,
        user_id,
        quantity
    ):
        rc = self.lib.inventory_move(
            ctypes.byref(self.db),
            product_id,
            source_location_id,
            destination_location_id,
            user_id,
            quantity
        )

        if rc == 1:
            return False

        if rc != 0:
            raise RuntimeError("Could not move inventory.")

        return True

    def list_users(self):
        max_users = 100

        users = (User * max_users)()
        user_count = ctypes.c_int()

        rc = self.lib.user_list(
            ctypes.byref(self.db),
            users,
            max_users,
            ctypes.byref(user_count)
        )

        if rc != 0:
            raise RuntimeError("Could not read users.")

        result = []

        for i in range(user_count.value):
            user = users[i]

            result.append({
                "id": user.id,
                "name": user.name.decode("utf-8"),
            })

        return result


    def list_locations(self):
        max_locations = 100

        locations = (Location * max_locations)()
        location_count = ctypes.c_int()

        rc = self.lib.location_list(
            ctypes.byref(self.db),
            locations,
            max_locations,
            ctypes.byref(location_count)
        )

        if rc != 0:
            raise RuntimeError("Could not read locations.")

        result = []

        for i in range(location_count.value):
            location = locations[i]

            result.append({
                "id": location.id,
                "name": location.name.decode("utf-8"),
                "parent_id": (
                    location.parent_id
                    if location.has_parent
                    else None
                ),
            })

        return result

    def list_products(self):
        max_products = 100

        products = (Product * max_products)()
        product_count = ctypes.c_int()

        rc = self.lib.product_list(
            ctypes.byref(self.db),
            products,
            max_products,
            ctypes.byref(product_count)
        )

        if rc != 0:
            raise RuntimeError("Could not read products.")

        result = []

        for i in range(product_count.value):
            product = products[i]

            result.append({
                "id": product.id,
                "name": product.name.decode("utf-8"),
                "brand": product.brand.decode("utf-8"),
                "unit": product.inventory_unit.decode("utf-8"),
                "low_stock_threshold": product.low_stock_threshold,
                "auto_add_grocery": bool(product.auto_add_grocery),
                "total_quantity": product.total_quantity,
                "is_low_stock": (
                    product.total_quantity <= product.low_stock_threshold
                ),
            })

        return result

    def update_product(
        self,
        product_id,
        name,
        brand,
        inventory_unit,
        low_stock_threshold,
        auto_add_grocery
    ):
        rc = self.lib.product_update(
            ctypes.byref(self.db),
            product_id,
            name.encode("utf-8"),
            brand.encode("utf-8") if brand else None,
            inventory_unit.encode("utf-8"),
            low_stock_threshold,
            1 if auto_add_grocery else 0
        )

        if rc == 1:
            return False

        if rc != 0:
            raise RuntimeError("Could not update product.")

        return True

    def create_product_with_package(
        self,
        name,
        brand,
        inventory_unit,
        low_stock_threshold,
        auto_add_grocery,
        barcode,
        package_quantity
    ):
        product_id = ctypes.c_int()

        rc = self.lib.product_create_with_package(
            ctypes.byref(self.db),
            name.encode("utf-8"),
            brand.encode("utf-8") if brand else None,
            inventory_unit.encode("utf-8"),
            low_stock_threshold,
            1 if auto_add_grocery else 0,
            barcode.encode("utf-8"),
            package_quantity,
            ctypes.byref(product_id)
        )

        if rc != 0:
            raise RuntimeError(
                "Could not create product and package."
            )

        return product_id.value

    def add_package(
        self,
        product_id,
        barcode,
        package_quantity
    ):
        rc = self.lib.product_add_package(
            ctypes.byref(self.db),
            product_id,
            barcode.encode("utf-8"),
            package_quantity
        )

        if rc != 0:
            raise RuntimeError(
                "Could not add package to product."
            )

    


class User(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("name", ctypes.c_char * 128),
    ]


class Location(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("parent_id", ctypes.c_int),
        ("has_parent", ctypes.c_int),
        ("name", ctypes.c_char * 128),
    ]
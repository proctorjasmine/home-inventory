from pathlib import Path
import os

import requests
from dotenv import load_dotenv
from flask import Flask, jsonify, request, render_template

from inventory import Inventory


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATABASE_PATH = PROJECT_ROOT / "data" / "inventory.db"

load_dotenv(PROJECT_ROOT / ".env")

SKYLIGHT_BRIDGE_URL = os.environ.get(
    "SKYLIGHT_BRIDGE_URL",
    "http://jdp5294.local:5000",
).rstrip("/")
SKYLIGHT_BRIDGE_TOKEN = os.environ.get("SKYLIGHT_BRIDGE_TOKEN", "")

app = Flask(__name__)


def get_product(inventory, product_id):
    return next(
        (p for p in inventory.list_products() if p["id"] == product_id),
        None,
    )


def maybe_add_low_stock_to_grocery(inventory, product_id):
    """Add an eligible low-stock product through the Raspberry Pi bridge."""
    product = get_product(inventory, product_id)

    if product is None:
        return {"attempted": False, "reason": "product_not_found"}
    if not product["auto_add_grocery"]:
        return {"attempted": False, "reason": "auto_add_disabled"}
    if not product["is_low_stock"]:
        return {"attempted": False, "reason": "not_low_stock"}

    if not SKYLIGHT_BRIDGE_TOKEN:
        app.logger.warning(
            "Grocery add skipped for %s: bridge token is not configured",
            product["name"],
        )
        return {"attempted": False, "reason": "bridge_not_configured"}

    try:
        response = requests.post(
            f"{SKYLIGHT_BRIDGE_URL}/grocery/add",
            headers={"X-NFC-Token": SKYLIGHT_BRIDGE_TOKEN},
            json={"name": product["name"]},
            timeout=5,
        )
    except requests.RequestException as error:
        app.logger.warning(
            "Skylight bridge unreachable for %s: %s",
            product["name"],
            error,
        )
        return {
            "attempted": True,
            "success": False,
            "reason": "bridge_unreachable",
        }

    if response.status_code in (200, 201):
        return {
            "attempted": True,
            "success": True,
            "added": response.status_code == 201,
            "already_present": response.status_code == 200,
            "status_code": response.status_code,
        }

    app.logger.warning(
        "Skylight grocery add failed for %s: HTTP %s: %s",
        product["name"],
        response.status_code,
        response.text.strip(),
    )
    return {
        "attempted": True,
        "success": False,
        "reason": "bridge_error",
        "status_code": response.status_code,
    }

@app.route("/")
def home():
    return render_template("index.html")

@app.route("/api/inventory", methods=["GET"])
def get_inventory():
    inventory = Inventory(DATABASE_PATH)

    try:
        items = inventory.list()

        return jsonify(items)

    finally:
        inventory.close()

@app.route("/api/history", methods=["GET"])
def get_history():
    inventory = Inventory(DATABASE_PATH)

    try:
        transactions = inventory.list_transactions()

        return jsonify(transactions)

    except RuntimeError as error:
        return jsonify({
            "error": str(error)
        }), 500

    finally:
        inventory.close()


@app.route("/api/products/barcode/<barcode>", methods=["GET"])
def get_product_by_barcode(barcode):
    inventory = Inventory(DATABASE_PATH)

    try:
        package = inventory.find_barcode(barcode)

        if package is None:
            return jsonify({
                "error": "Barcode not found"
            }), 404

        return jsonify(package)

    finally:
        inventory.close()


@app.route("/api/inventory/add", methods=["POST"])
def add_inventory():
    data = request.get_json(silent=True)

    if data is None:
        return jsonify({
            "error": "Request body must be JSON"
        }), 400

    barcode = data.get("barcode")
    location_id = data.get("location_id")
    user_id = data.get("user_id")

    if not barcode or location_id is None or user_id is None:
        return jsonify({
            "error": "barcode, location_id, and user_id are required"
        }), 400

    inventory = Inventory(DATABASE_PATH)

    try:
        package = inventory.find_barcode(barcode)

        if package is None:
            return jsonify({
                "error": "Barcode not found"
            }), 404

        inventory.add(
            product_id=package["product_id"],
            location_id=location_id,
            user_id=user_id,
            quantity=package["package_quantity"]
        )

        new_quantity = inventory.get_quantity(
            product_id=package["product_id"],
            location_id=location_id
        )

        return jsonify({
            "message": "Inventory added",
            "product": package["product"],
            "added": package["package_quantity"],
            "unit": package["unit"],
            "new_quantity": new_quantity
        })

    except RuntimeError as error:
        return jsonify({
            "error": str(error)
        }), 400

    finally:
        inventory.close()


@app.route("/api/inventory/remove", methods=["POST"])
def remove_inventory():
    data = request.get_json(silent=True)

    if data is None:
        return jsonify({
            "error": "Request body must be JSON"
        }), 400

    product_id = data.get("product_id")
    location_id = data.get("location_id")
    user_id = data.get("user_id")
    quantity = data.get("quantity", 1)

    if product_id is None or location_id is None or user_id is None:
        return jsonify({
            "error": "product_id, location_id, and user_id are required"
        }), 400

    if not isinstance(quantity, int) or quantity <= 0:
        return jsonify({
            "error": "quantity must be a positive integer"
        }), 400

    inventory = Inventory(DATABASE_PATH)

    try:
        removed = inventory.remove(
            product_id=product_id,
            location_id=location_id,
            user_id=user_id,
            quantity=quantity
        )

        if not removed:
            return jsonify({
                "error": "Not enough inventory"
            }), 409

        new_quantity = inventory.get_quantity(
            product_id=product_id,
            location_id=location_id
        )

        grocery = maybe_add_low_stock_to_grocery(
            inventory,
            product_id,
        )

        return jsonify({
            "message": "Inventory removed",
            "removed": quantity,
            "new_quantity": new_quantity,
            "grocery": grocery
        })

    except RuntimeError as error:
        return jsonify({
            "error": str(error)
        }), 400

    finally:
        inventory.close()

@app.route("/api/inventory/adjust", methods=["POST"])
def adjust_inventory():
    data = request.get_json(silent=True)

    if data is None:
        return jsonify({"error": "Request body must be JSON"}), 400

    product_id = data.get("product_id")
    location_id = data.get("location_id")
    user_id = data.get("user_id")
    new_quantity = data.get("new_quantity")

    if (
        product_id is None or
        location_id is None or
        user_id is None or
        new_quantity is None
    ):
        return jsonify({
            "error":
                "product_id, location_id, user_id, and new_quantity "
                "are required"
        }), 400

    if not isinstance(new_quantity, int) or new_quantity < 0:
        return jsonify({
            "error": "new_quantity must be a non-negative integer"
        }), 400

    inventory = Inventory(DATABASE_PATH)

    try:
        changed = inventory.adjust(
            product_id=product_id,
            location_id=location_id,
            user_id=user_id,
            new_quantity=new_quantity
        )

        grocery = (
            maybe_add_low_stock_to_grocery(inventory, product_id)
            if changed
            else {"attempted": False, "reason": "inventory_unchanged"}
        )

        return jsonify({
            "message": (
                "Inventory adjusted"
                if changed
                else "Inventory already matched physical count"
            ),
            "changed": changed,
            "new_quantity": new_quantity,
            "grocery": grocery
        })

    except RuntimeError as error:
        return jsonify({"error": str(error)}), 400

    finally:
        inventory.close()


@app.route("/api/inventory/move", methods=["POST"])
def move_inventory():
    data = request.get_json(silent=True)

    if data is None:
        return jsonify({"error": "Request body must be JSON"}), 400

    product_id = data.get("product_id")
    source_location_id = data.get("source_location_id")
    destination_location_id = data.get("destination_location_id")
    user_id = data.get("user_id")
    quantity = data.get("quantity")

    if (
        product_id is None or
        source_location_id is None or
        destination_location_id is None or
        user_id is None or
        quantity is None
    ):
        return jsonify({
            "error":
                "product_id, source_location_id, "
                "destination_location_id, user_id, and quantity "
                "are required"
        }), 400

    if not isinstance(quantity, int) or quantity <= 0:
        return jsonify({
            "error": "quantity must be a positive integer"
        }), 400

    if source_location_id == destination_location_id:
        return jsonify({
            "error": "Source and destination must be different"
        }), 400

    inventory = Inventory(DATABASE_PATH)

    try:
        moved = inventory.move(
            product_id=product_id,
            source_location_id=source_location_id,
            destination_location_id=destination_location_id,
            user_id=user_id,
            quantity=quantity
        )

        if not moved:
            return jsonify({
                "error": "Not enough inventory at source location"
            }), 409

        return jsonify({
            "message": "Inventory moved",
            "moved": quantity,
            "source_quantity": inventory.get_quantity(
                product_id,
                source_location_id
            ),
            "destination_quantity": inventory.get_quantity(
                product_id,
                destination_location_id
            )
        })

    except RuntimeError as error:
        return jsonify({"error": str(error)}), 400

    finally:
        inventory.close()


@app.route("/api/users", methods=["GET"])
def get_users():
    inventory = Inventory(DATABASE_PATH)

    try:
        return jsonify(inventory.list_users())

    finally:
        inventory.close()


@app.route("/api/locations", methods=["GET"])
def get_locations():
    inventory = Inventory(DATABASE_PATH)

    try:
        return jsonify(inventory.list_locations())

    finally:
        inventory.close()

@app.route("/api/products", methods=["GET"])
def get_products():
    inventory = Inventory(DATABASE_PATH)

    try:
        return jsonify(inventory.list_products())

    finally:
        inventory.close()


@app.route("/api/low-stock", methods=["GET"])
def get_low_stock():
    inventory = Inventory(DATABASE_PATH)

    try:
        products = inventory.list_products()
        return jsonify([
            product
            for product in products
            if product["is_low_stock"]
        ])

    finally:
        inventory.close()


@app.route("/api/products/<int:product_id>", methods=["PATCH"])
def update_product(product_id):
    data = request.get_json(silent=True)

    if data is None:
        return jsonify({"error": "Request body must be JSON"}), 400

    inventory = Inventory(DATABASE_PATH)

    try:
        products = inventory.list_products()
        current = next(
            (
                product for product in products
                if product["id"] == product_id
            ),
            None
        )

        if current is None:
            return jsonify({"error": "Product not found"}), 404

        name = str(data.get("name", current["name"])).strip()
        brand = str(data.get("brand", current["brand"])).strip()
        inventory_unit = str(
            data.get("inventory_unit", current["unit"])
        ).strip()

        try:
            low_stock_threshold = int(
                data.get(
                    "low_stock_threshold",
                    current["low_stock_threshold"]
                )
            )
        except (TypeError, ValueError):
            return jsonify({
                "error": "Low-stock threshold must be a number"
            }), 400

        auto_add_grocery = bool(
            data.get(
                "auto_add_grocery",
                current["auto_add_grocery"]
            )
        )

        if not name:
            return jsonify({"error": "Product name is required"}), 400

        if not inventory_unit:
            return jsonify({
                "error": "Inventory unit is required"
            }), 400

        if low_stock_threshold < 0:
            return jsonify({
                "error": "Low-stock threshold cannot be negative"
            }), 400

        updated = inventory.update_product(
            product_id=product_id,
            name=name,
            brand=brand,
            inventory_unit=inventory_unit,
            low_stock_threshold=low_stock_threshold,
            auto_add_grocery=auto_add_grocery
        )

        if not updated:
            return jsonify({"error": "Product not found"}), 404

        refreshed = next(
            product
            for product in inventory.list_products()
            if product["id"] == product_id
        )

        grocery = maybe_add_low_stock_to_grocery(
            inventory,
            product_id,
        )

        response_body = dict(refreshed)
        response_body["grocery"] = grocery

        return jsonify(response_body)

    except RuntimeError as error:
        return jsonify({"error": str(error)}), 400

    finally:
        inventory.close()


@app.route("/api/product-packages", methods=["POST"])
def add_product_package():
    data = request.get_json(silent=True)

    if data is None:
        return jsonify({
            "error": "Request body must be JSON"
        }), 400

    product_id = data.get("product_id")
    barcode = str(
        data.get("barcode", "")
    ).strip()

    try:
        package_quantity = int(
            data.get("package_quantity", 0)
        )
    except (TypeError, ValueError):
        return jsonify({
            "error": "Package quantity must be a number."
        }), 400

    if product_id is None:
        return jsonify({
            "error": "product_id is required."
        }), 400

    if not barcode:
        return jsonify({
            "error": "Barcode is required."
        }), 400

    if package_quantity <= 0:
        return jsonify({
            "error": "Package quantity must be greater than zero."
        }), 400

    inventory = Inventory(DATABASE_PATH)

    try:
        inventory.add_package(
            product_id=product_id,
            barcode=barcode,
            package_quantity=package_quantity
        )

        return jsonify({
            "message": "Package added.",
            "product_id": product_id,
            "barcode": barcode,
            "package_quantity": package_quantity
        }), 201

    except RuntimeError:
        return jsonify({
            "error":
                "Could not add package. "
                "The barcode may already exist."
        }), 409

    finally:
        inventory.close()


@app.route("/scan")
def scan():
    return render_template("scan.html")

@app.route("/history")
def history():
    return render_template("history.html")


@app.post("/api/products")
def create_product():
    data = request.get_json(silent=True)

    if not data:
        return jsonify({
            "error": "JSON body is required."
        }), 400

    name = str(data.get("name", "")).strip()
    brand = str(data.get("brand", "")).strip()
    inventory_unit = str(
        data.get("inventory_unit", "")
    ).strip()

    barcode = str(
        data.get("barcode", "")
    ).strip()

    try:
        package_quantity = int(
            data.get("package_quantity", 0)
        )

        low_stock_threshold = int(
            data.get("low_stock_threshold", 0)
        )
    except (TypeError, ValueError):
        return jsonify({
            "error":
                "Package quantity and low-stock "
                "threshold must be numbers."
        }), 400

    auto_add_grocery = bool(
        data.get("auto_add_grocery", False)
    )

    if not name:
        return jsonify({
            "error": "Product name is required."
        }), 400

    if not inventory_unit:
        return jsonify({
            "error": "Inventory unit is required."
        }), 400

    if not barcode:
        return jsonify({
            "error": "Barcode is required."
        }), 400

    if package_quantity <= 0:
        return jsonify({
            "error":
                "Package quantity must be greater than zero."
        }), 400

    if low_stock_threshold < 0:
        return jsonify({
            "error":
                "Low-stock threshold cannot be negative."
        }), 400

    db = Inventory(DATABASE_PATH)

    try:
        product_id = (
            db.create_product_with_package(
                name=name,
                brand=brand,
                inventory_unit=inventory_unit,
                low_stock_threshold=
                    low_stock_threshold,
                auto_add_grocery=
                    auto_add_grocery,
                barcode=barcode,
                package_quantity=
                    package_quantity
            )
        )

        return jsonify({
            "product_id": product_id,
            "barcode": barcode,
            "message": "Product created."
        }), 201

    except RuntimeError:
        return jsonify({
            "error":
                "Could not create product. "
                "The barcode may already exist."
        }), 409

    finally:
        db.close()

if __name__ == "__main__":
    app.run(
        host="0.0.0.0",
        port=5000,
        debug=True,
        ssl_context=(
            "certs/dev-cert.pem",
            "certs/dev-key.pem"
        )
    )
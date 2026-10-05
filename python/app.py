from pathlib import Path

from flask import Flask, jsonify, request

from inventory import Inventory


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATABASE_PATH = PROJECT_ROOT / "data" / "inventory.db"

app = Flask(__name__)


@app.route("/api/inventory", methods=["GET"])
def get_inventory():
    inventory = Inventory(DATABASE_PATH)

    try:
        items = inventory.list()

        return jsonify(items)

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

        
if __name__ == "__main__":
    app.run(debug=True)
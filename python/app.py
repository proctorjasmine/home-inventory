from pathlib import Path

from flask import Flask, jsonify

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


if __name__ == "__main__":
    app.run(debug=True)
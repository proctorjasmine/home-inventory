from pathlib import Path

from inventory import Inventory


PROJECT_ROOT = Path(__file__).resolve().parent.parent
DATABASE_PATH = PROJECT_ROOT / "data" / "inventory.db"


inventory = Inventory(DATABASE_PATH)

items = inventory.list()

for item in items:
    print(item)

inventory.close()
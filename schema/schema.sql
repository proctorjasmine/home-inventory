CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);


CREATE TABLE IF NOT EXISTS products (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL,
    brand TEXT,

    inventory_unit TEXT NOT NULL DEFAULT 'item',

    low_stock_threshold INTEGER NOT NULL DEFAULT 0
        CHECK (low_stock_threshold >= 0),

    auto_add_grocery INTEGER NOT NULL DEFAULT 0
        CHECK (auto_add_grocery IN (0, 1))
);


CREATE TABLE IF NOT EXISTS product_packages (
    id INTEGER PRIMARY KEY,

    product_id INTEGER NOT NULL,

    barcode TEXT NOT NULL UNIQUE,

    package_quantity INTEGER NOT NULL
        CHECK (package_quantity > 0),

    FOREIGN KEY (product_id)
        REFERENCES products(id)
);


CREATE TABLE IF NOT EXISTS locations (
    id INTEGER PRIMARY KEY,

    name TEXT NOT NULL,

    parent_id INTEGER,

    FOREIGN KEY (parent_id)
        REFERENCES locations(id)
);


CREATE TABLE IF NOT EXISTS inventory (
    id INTEGER PRIMARY KEY,

    product_id INTEGER NOT NULL,
    location_id INTEGER NOT NULL,

    quantity INTEGER NOT NULL DEFAULT 0
        CHECK (quantity >= 0),

    FOREIGN KEY (product_id)
        REFERENCES products(id),

    FOREIGN KEY (location_id)
        REFERENCES locations(id),

    UNIQUE (product_id, location_id)
);


CREATE TABLE IF NOT EXISTS transactions (
    id INTEGER PRIMARY KEY,

    product_id INTEGER NOT NULL,
    location_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,

    transaction_type TEXT NOT NULL
        CHECK (
            transaction_type IN (
                'restock',
                'consume',
                'adjustment',
                'move'
            )
        ),

    quantity_change INTEGER NOT NULL
        CHECK (quantity_change != 0),

    previous_quantity INTEGER,
    new_quantity INTEGER,

    destination_location_id INTEGER,

    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (product_id)
        REFERENCES products(id),

    FOREIGN KEY (location_id)
        REFERENCES locations(id),

    FOREIGN KEY (destination_location_id)
        REFERENCES locations(id),

    FOREIGN KEY (user_id)
        REFERENCES users(id)
);
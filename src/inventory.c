#include <stdio.h>
#include "inventory.h"


int inventory_add(Database *db, int product_id, int location_id, int user_id, int quantity){
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (quantity <= 0){
        fprintf(stderr, "Quantity must be greater than zero.\n");
        return -1;
    }

    /* Start database transaction */
    rc = sqlite3_exec(db->connection,"BEGIN TRANSACTION;", NULL, NULL, NULL);

    if (rc != SQLITE_OK){
        fprintf(stderr,
                "Could not begin transaction: %s\n",
                sqlite3_errmsg(db->connection));

        return -1;
    }

    // Add quantity to inventory.
    // If this product/location combination doesn't exist,
    // create it.
    // If it does exist, increase its quantity.

    const char *inventory_sql =
        "INSERT INTO inventory "
        "(product_id, location_id, quantity) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(product_id, location_id) "
        "DO UPDATE SET quantity = quantity + excluded.quantity;";

    rc = sqlite3_prepare_v2(db->connection, inventory_sql,-1, &stmt, NULL);

    if (rc != SQLITE_OK){
        fprintf(stderr,"Could not prepare inventory update: %s\n",sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, location_id);
    sqlite3_bind_int(stmt, 3, quantity);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (rc != SQLITE_DONE)
    {
        fprintf(stderr,
                "Could not update inventory: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    // Record the change in transaction history.
    const char *transaction_sql =
        "INSERT INTO transactions "
        "(product_id, location_id, user_id, quantity_change) "
        "VALUES (?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(
        db->connection,
        transaction_sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not prepare transaction record: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, location_id);
    sqlite3_bind_int(stmt, 3, user_id);
    sqlite3_bind_int(stmt, 4, quantity);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (rc != SQLITE_DONE)
    {
        fprintf(stderr,
                "Could not record transaction: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    // Both operations succeeded.
    rc = sqlite3_exec(
        db->connection,
        "COMMIT;",
        NULL,
        NULL,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not commit transaction: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    return 0;
}

int inventory_remove(Database *db, int product_id,int location_id, int user_id, int quantity){
    sqlite3_stmt *stmt = NULL;
    int rc;

    if (quantity <= 0){
        fprintf(stderr, "Quantity must be greater than zero.\n");
        return -1;
    }

    rc = sqlite3_exec(
        db->connection,
        "BEGIN TRANSACTION;",
        NULL,
        NULL,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not begin transaction: %s\n",
                sqlite3_errmsg(db->connection));

        return -1;
    }

    const char *inventory_sql =
        "UPDATE inventory "
        "SET quantity = quantity - ? "
        "WHERE product_id = ? "
        "AND location_id = ? "
        "AND quantity >= ?;";

    rc = sqlite3_prepare_v2(
        db->connection,
        inventory_sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not prepare inventory removal: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_int(stmt, 2, product_id);
    sqlite3_bind_int(stmt, 3, location_id);
    sqlite3_bind_int(stmt, 4, quantity);

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (rc != SQLITE_DONE)
    {
        fprintf(stderr,
                "Could not remove inventory: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    // Did the UPDATE actually find a row that
    // had enough inventory?
    if (sqlite3_changes(db->connection) == 0)
    {
        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return 1;
    }

    const char *transaction_sql =
        "INSERT INTO transactions "
        "(product_id, location_id, user_id, quantity_change) "
        "VALUES (?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(
        db->connection,
        transaction_sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not prepare transaction record: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, location_id);
    sqlite3_bind_int(stmt, 3, user_id);
    sqlite3_bind_int(stmt, 4, -quantity);  // Removal is recorded as a negative change.

    rc = sqlite3_step(stmt);

    sqlite3_finalize(stmt);
    stmt = NULL;

    if (rc != SQLITE_DONE)
    {
        fprintf(stderr,
                "Could not record transaction: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    rc = sqlite3_exec(
        db->connection,
        "COMMIT;",
        NULL,
        NULL,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not commit transaction: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_exec(
            db->connection,
            "ROLLBACK;",
            NULL,
            NULL,
            NULL
        );

        return -1;
    }

    return 0;
}

int inventory_get_quantity(
    Database *db,
    int product_id,
    int location_id,
    int *quantity
)
{
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT quantity "
        "FROM inventory "
        "WHERE product_id = ? "
        "AND location_id = ?;";

    int rc = sqlite3_prepare_v2(
        db->connection,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not prepare inventory query: %s\n",
                sqlite3_errmsg(db->connection));

        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, location_id);

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        *quantity = sqlite3_column_int(stmt, 0);

        sqlite3_finalize(stmt);

        return 0;
    }

    if (rc == SQLITE_DONE)
    {
        // No inventory row means we have zero
        // of this product at this location.
        *quantity = 0;

        sqlite3_finalize(stmt);

        return 0;
    }

    fprintf(stderr,
            "Could not read inventory: %s\n",
            sqlite3_errmsg(db->connection));

    sqlite3_finalize(stmt);

    return -1;
}

int inventory_list(
    Database *db,
    InventoryItem *items,
    int max_items,
    int *item_count
)
{
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT "
        "i.product_id, "
        "i.location_id, "
        "p.name, "
        "p.inventory_unit, "
        "l.name, "
        "i.quantity "
        "FROM inventory i "
        "JOIN products p ON i.product_id = p.id "
        "JOIN locations l ON i.location_id = l.id "
        "WHERE i.quantity > 0 "
        "ORDER BY l.name, p.name;";

    int rc = sqlite3_prepare_v2(
        db->connection,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(stderr,
                "Could not prepare inventory list: %s\n",
                sqlite3_errmsg(db->connection));

        return -1;
    }

    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (count >= max_items)
        {
            break;
        }

        InventoryItem *item = &items[count];

        item->product_id =
            sqlite3_column_int(stmt, 0);

        item->location_id =
            sqlite3_column_int(stmt, 1);

        snprintf(
            item->product_name,
            sizeof(item->product_name),
            "%s",
            sqlite3_column_text(stmt, 2)
        );

        snprintf(
            item->inventory_unit,
            sizeof(item->inventory_unit),
            "%s",
            sqlite3_column_text(stmt, 3)
        );

        snprintf(
            item->location_name,
            sizeof(item->location_name),
            "%s",
            sqlite3_column_text(stmt, 4)
        );

        item->quantity =
            sqlite3_column_int(stmt, 5);

        count++;
    }

    if (rc != SQLITE_DONE && count < max_items)
    {
        fprintf(stderr,
                "Could not read inventory list: %s\n",
                sqlite3_errmsg(db->connection));

        sqlite3_finalize(stmt);

        return -1;
    }

    sqlite3_finalize(stmt);

    *item_count = count;

    return 0;
}
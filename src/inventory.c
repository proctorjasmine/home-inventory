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

int inventory_transaction_list(
    Database *db,
    InventoryTransaction *transactions,
    int max_transactions,
    int *transaction_count
)
{
    if (
        db == NULL ||
        db->connection == NULL ||
        transactions == NULL ||
        transaction_count == NULL ||
        max_transactions <= 0
    )
    {
        return -1;
    }

    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "SELECT "
        "t.id, "
        "t.product_id, "
        "t.location_id, "
        "t.user_id, "
        "p.name, "
        "p.inventory_unit, "
        "l.name, "
        "u.name, "
        "t.quantity_change, "
        "t.created_at "
        "FROM transactions t "
        "JOIN products p ON t.product_id = p.id "
        "JOIN locations l ON t.location_id = l.id "
        "JOIN users u ON t.user_id = u.id "
        "ORDER BY t.created_at DESC, t.id DESC;";

    int rc = sqlite3_prepare_v2(
        db->connection,
        sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(
            stderr,
            "Could not prepare transaction history: %s\n",
            sqlite3_errmsg(db->connection)
        );

        return -1;
    }

    int count = 0;

    while (
        count < max_transactions &&
        (rc = sqlite3_step(stmt)) == SQLITE_ROW
    )
    {
        InventoryTransaction *transaction =
            &transactions[count];

        transaction->id =
            sqlite3_column_int(stmt, 0);

        transaction->product_id =
            sqlite3_column_int(stmt, 1);

        transaction->location_id =
            sqlite3_column_int(stmt, 2);

        transaction->user_id =
            sqlite3_column_int(stmt, 3);

        const unsigned char *product_name =
            sqlite3_column_text(stmt, 4);

        const unsigned char *inventory_unit =
            sqlite3_column_text(stmt, 5);

        const unsigned char *location_name =
            sqlite3_column_text(stmt, 6);

        const unsigned char *user_name =
            sqlite3_column_text(stmt, 7);

        snprintf(
            transaction->product_name,
            sizeof(transaction->product_name),
            "%s",
            product_name != NULL
                ? (const char *)product_name
                : ""
        );

        snprintf(
            transaction->inventory_unit,
            sizeof(transaction->inventory_unit),
            "%s",
            inventory_unit != NULL
                ? (const char *)inventory_unit
                : ""
        );

        snprintf(
            transaction->location_name,
            sizeof(transaction->location_name),
            "%s",
            location_name != NULL
                ? (const char *)location_name
                : ""
        );

        snprintf(
            transaction->user_name,
            sizeof(transaction->user_name),
            "%s",
            user_name != NULL
                ? (const char *)user_name
                : ""
        );

        transaction->quantity_change =
            sqlite3_column_int(stmt, 8);

        const unsigned char *created_at =
            sqlite3_column_text(stmt, 9);

        snprintf(
            transaction->created_at,
            sizeof(transaction->created_at),
            "%s",
            created_at != NULL
                ? (const char *)created_at
                : ""
        );

        count++;
    }

    if (
        rc != SQLITE_DONE &&
        count < max_transactions
    )
    {
        fprintf(
            stderr,
            "Could not read transaction history: %s\n",
            sqlite3_errmsg(db->connection)
        );

        sqlite3_finalize(stmt);

        return -1;
    }

    sqlite3_finalize(stmt);

    *transaction_count = count;

    return 0;
}
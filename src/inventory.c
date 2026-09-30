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
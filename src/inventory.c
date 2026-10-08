#include <stdio.h>
#include <string.h>
#include "inventory.h"

static void rollback(Database *db)
{
    sqlite3_exec(db->connection, "ROLLBACK;", NULL, NULL, NULL);
}

static int begin_transaction(Database *db)
{
    int rc = sqlite3_exec(
        db->connection,
        "BEGIN TRANSACTION;",
        NULL,
        NULL,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(
            stderr,
            "Could not begin transaction: %s\n",
            sqlite3_errmsg(db->connection)
        );
        return -1;
    }

    return 0;
}

static int commit_transaction(Database *db)
{
    int rc = sqlite3_exec(
        db->connection,
        "COMMIT;",
        NULL,
        NULL,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(
            stderr,
            "Could not commit transaction: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    return 0;
}

static int record_transaction(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    const char *transaction_type,
    int quantity_change,
    int has_previous_quantity,
    int previous_quantity,
    int has_new_quantity,
    int new_quantity,
    int destination_location_id
)
{
    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "INSERT INTO transactions ("
        "product_id, location_id, user_id, transaction_type, "
        "quantity_change, previous_quantity, new_quantity, "
        "destination_location_id"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

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
            "Could not prepare transaction record: %s\n",
            sqlite3_errmsg(db->connection)
        );
        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, location_id);
    sqlite3_bind_int(stmt, 3, user_id);
    sqlite3_bind_text(stmt, 4, transaction_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, quantity_change);

    if (has_previous_quantity)
        sqlite3_bind_int(stmt, 6, previous_quantity);
    else
        sqlite3_bind_null(stmt, 6);

    if (has_new_quantity)
        sqlite3_bind_int(stmt, 7, new_quantity);
    else
        sqlite3_bind_null(stmt, 7);

    if (destination_location_id > 0)
        sqlite3_bind_int(stmt, 8, destination_location_id);
    else
        sqlite3_bind_null(stmt, 8);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(
            stderr,
            "Could not record transaction: %s\n",
            sqlite3_errmsg(db->connection)
        );
        return -1;
    }

    return 0;
}

int inventory_add(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    int quantity
)
{
    if (
        db == NULL ||
        db->connection == NULL ||
        product_id <= 0 ||
        location_id <= 0 ||
        user_id <= 0 ||
        quantity <= 0
    )
    {
        fprintf(stderr, "Invalid inventory add request.\n");
        return -1;
    }

    if (begin_transaction(db) != 0)
        return -1;

    sqlite3_stmt *stmt = NULL;

    const char *inventory_sql =
        "INSERT INTO inventory "
        "(product_id, location_id, quantity) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(product_id, location_id) "
        "DO UPDATE SET quantity = quantity + excluded.quantity;";

    int rc = sqlite3_prepare_v2(
        db->connection,
        inventory_sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(
            stderr,
            "Could not prepare inventory update: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, location_id);
    sqlite3_bind_int(stmt, 3, quantity);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(
            stderr,
            "Could not update inventory: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    if (
        record_transaction(
            db,
            product_id,
            location_id,
            user_id,
            "restock",
            quantity,
            0,
            0,
            0,
            0,
            0
        ) != 0
    )
    {
        rollback(db);
        return -1;
    }

    return commit_transaction(db);
}

int inventory_remove(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    int quantity
)
{
    if (
        db == NULL ||
        db->connection == NULL ||
        product_id <= 0 ||
        location_id <= 0 ||
        user_id <= 0 ||
        quantity <= 0
    )
    {
        fprintf(stderr, "Invalid inventory removal request.\n");
        return -1;
    }

    if (begin_transaction(db) != 0)
        return -1;

    sqlite3_stmt *stmt = NULL;

    const char *inventory_sql =
        "UPDATE inventory "
        "SET quantity = quantity - ? "
        "WHERE product_id = ? "
        "AND location_id = ? "
        "AND quantity >= ?;";

    int rc = sqlite3_prepare_v2(
        db->connection,
        inventory_sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(
            stderr,
            "Could not prepare inventory removal: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_int(stmt, 2, product_id);
    sqlite3_bind_int(stmt, 3, location_id);
    sqlite3_bind_int(stmt, 4, quantity);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(
            stderr,
            "Could not remove inventory: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    if (sqlite3_changes(db->connection) == 0)
    {
        rollback(db);
        return 1;
    }

    if (
        record_transaction(
            db,
            product_id,
            location_id,
            user_id,
            "consume",
            -quantity,
            0,
            0,
            0,
            0,
            0
        ) != 0
    )
    {
        rollback(db);
        return -1;
    }

    return commit_transaction(db);
}

int inventory_get_quantity(
    Database *db,
    int product_id,
    int location_id,
    int *quantity
)
{
    if (
        db == NULL ||
        db->connection == NULL ||
        quantity == NULL ||
        product_id <= 0 ||
        location_id <= 0
    )
    {
        return -1;
    }

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
        fprintf(
            stderr,
            "Could not prepare inventory query: %s\n",
            sqlite3_errmsg(db->connection)
        );
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
        *quantity = 0;
        sqlite3_finalize(stmt);
        return 0;
    }

    fprintf(
        stderr,
        "Could not read inventory: %s\n",
        sqlite3_errmsg(db->connection)
    );

    sqlite3_finalize(stmt);
    return -1;
}

int inventory_adjust(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    int new_quantity
)
{
    if (
        db == NULL ||
        db->connection == NULL ||
        product_id <= 0 ||
        location_id <= 0 ||
        user_id <= 0 ||
        new_quantity < 0
    )
    {
        fprintf(stderr, "Invalid inventory adjustment request.\n");
        return -1;
    }

    if (begin_transaction(db) != 0)
        return -1;

    int previous_quantity = 0;

    if (
        inventory_get_quantity(
            db,
            product_id,
            location_id,
            &previous_quantity
        ) != 0
    )
    {
        rollback(db);
        return -1;
    }

    if (previous_quantity == new_quantity)
    {
        rollback(db);
        return 1;
    }

    sqlite3_stmt *stmt = NULL;

    const char *sql =
        "INSERT INTO inventory "
        "(product_id, location_id, quantity) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(product_id, location_id) "
        "DO UPDATE SET quantity = excluded.quantity;";

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
            "Could not prepare inventory adjustment: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, location_id);
    sqlite3_bind_int(stmt, 3, new_quantity);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(
            stderr,
            "Could not adjust inventory: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    int quantity_change = new_quantity - previous_quantity;

    if (
        record_transaction(
            db,
            product_id,
            location_id,
            user_id,
            "adjustment",
            quantity_change,
            1,
            previous_quantity,
            1,
            new_quantity,
            0
        ) != 0
    )
    {
        rollback(db);
        return -1;
    }

    return commit_transaction(db);
}

int inventory_move(
    Database *db,
    int product_id,
    int source_location_id,
    int destination_location_id,
    int user_id,
    int quantity
)
{
    if (
        db == NULL ||
        db->connection == NULL ||
        product_id <= 0 ||
        source_location_id <= 0 ||
        destination_location_id <= 0 ||
        user_id <= 0 ||
        quantity <= 0 ||
        source_location_id == destination_location_id
    )
    {
        fprintf(stderr, "Invalid inventory move request.\n");
        return -1;
    }

    if (begin_transaction(db) != 0)
        return -1;

    sqlite3_stmt *stmt = NULL;

    const char *remove_sql =
        "UPDATE inventory "
        "SET quantity = quantity - ? "
        "WHERE product_id = ? "
        "AND location_id = ? "
        "AND quantity >= ?;";

    int rc = sqlite3_prepare_v2(
        db->connection,
        remove_sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(
            stderr,
            "Could not prepare move from source: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, quantity);
    sqlite3_bind_int(stmt, 2, product_id);
    sqlite3_bind_int(stmt, 3, source_location_id);
    sqlite3_bind_int(stmt, 4, quantity);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(
            stderr,
            "Could not remove inventory from source: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    if (sqlite3_changes(db->connection) == 0)
    {
        rollback(db);
        return 1;
    }

    const char *add_sql =
        "INSERT INTO inventory "
        "(product_id, location_id, quantity) "
        "VALUES (?, ?, ?) "
        "ON CONFLICT(product_id, location_id) "
        "DO UPDATE SET quantity = quantity + excluded.quantity;";

    rc = sqlite3_prepare_v2(
        db->connection,
        add_sql,
        -1,
        &stmt,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        fprintf(
            stderr,
            "Could not prepare move to destination: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, product_id);
    sqlite3_bind_int(stmt, 2, destination_location_id);
    sqlite3_bind_int(stmt, 3, quantity);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        fprintf(
            stderr,
            "Could not add inventory to destination: %s\n",
            sqlite3_errmsg(db->connection)
        );
        rollback(db);
        return -1;
    }

    if (
        record_transaction(
            db,
            product_id,
            source_location_id,
            user_id,
            "move",
            quantity,
            0,
            0,
            0,
            0,
            destination_location_id
        ) != 0
    )
    {
        rollback(db);
        return -1;
    }

    return commit_transaction(db);
}

int inventory_list(
    Database *db,
    InventoryItem *items,
    int max_items,
    int *item_count
)
{
    if (
        db == NULL ||
        db->connection == NULL ||
        items == NULL ||
        item_count == NULL ||
        max_items <= 0
    )
    {
        return -1;
    }

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
        fprintf(
            stderr,
            "Could not prepare inventory list: %s\n",
            sqlite3_errmsg(db->connection)
        );
        return -1;
    }

    int count = 0;

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (count >= max_items)
            break;

        InventoryItem *item = &items[count];

        item->product_id = sqlite3_column_int(stmt, 0);
        item->location_id = sqlite3_column_int(stmt, 1);

        const unsigned char *product_name =
            sqlite3_column_text(stmt, 2);
        const unsigned char *inventory_unit =
            sqlite3_column_text(stmt, 3);
        const unsigned char *location_name =
            sqlite3_column_text(stmt, 4);

        snprintf(
            item->product_name,
            sizeof(item->product_name),
            "%s",
            product_name != NULL ? (const char *)product_name : ""
        );

        snprintf(
            item->inventory_unit,
            sizeof(item->inventory_unit),
            "%s",
            inventory_unit != NULL ? (const char *)inventory_unit : ""
        );

        snprintf(
            item->location_name,
            sizeof(item->location_name),
            "%s",
            location_name != NULL ? (const char *)location_name : ""
        );

        item->quantity = sqlite3_column_int(stmt, 5);
        count++;
    }

    if (rc != SQLITE_DONE && count < max_items)
    {
        fprintf(
            stderr,
            "Could not read inventory list: %s\n",
            sqlite3_errmsg(db->connection)
        );
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
        "COALESCE("
            "t.transaction_type, "
            "CASE "
                "WHEN t.quantity_change > 0 THEN 'restock' "
                "ELSE 'consume' "
            "END"
        "), "
        "t.quantity_change, "
        "COALESCE(t.previous_quantity, -1), "
        "COALESCE(t.new_quantity, -1), "
        "COALESCE(t.destination_location_id, 0), "
        "COALESCE(dl.name, ''), "
        "t.created_at "
        "FROM transactions t "
        "JOIN products p ON t.product_id = p.id "
        "JOIN locations l ON t.location_id = l.id "
        "JOIN users u ON t.user_id = u.id "
        "LEFT JOIN locations dl "
        "ON t.destination_location_id = dl.id "
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

        transaction->id = sqlite3_column_int(stmt, 0);
        transaction->product_id = sqlite3_column_int(stmt, 1);
        transaction->location_id = sqlite3_column_int(stmt, 2);
        transaction->user_id = sqlite3_column_int(stmt, 3);

        const unsigned char *product_name =
            sqlite3_column_text(stmt, 4);
        const unsigned char *inventory_unit =
            sqlite3_column_text(stmt, 5);
        const unsigned char *location_name =
            sqlite3_column_text(stmt, 6);
        const unsigned char *user_name =
            sqlite3_column_text(stmt, 7);
        const unsigned char *transaction_type =
            sqlite3_column_text(stmt, 8);
        const unsigned char *destination_location_name =
            sqlite3_column_text(stmt, 13);
        const unsigned char *created_at =
            sqlite3_column_text(stmt, 14);

        snprintf(
            transaction->product_name,
            sizeof(transaction->product_name),
            "%s",
            product_name != NULL ? (const char *)product_name : ""
        );

        snprintf(
            transaction->inventory_unit,
            sizeof(transaction->inventory_unit),
            "%s",
            inventory_unit != NULL ? (const char *)inventory_unit : ""
        );

        snprintf(
            transaction->location_name,
            sizeof(transaction->location_name),
            "%s",
            location_name != NULL ? (const char *)location_name : ""
        );

        snprintf(
            transaction->user_name,
            sizeof(transaction->user_name),
            "%s",
            user_name != NULL ? (const char *)user_name : ""
        );

        snprintf(
            transaction->transaction_type,
            sizeof(transaction->transaction_type),
            "%s",
            transaction_type != NULL ? (const char *)transaction_type : ""
        );

        transaction->quantity_change =
            sqlite3_column_int(stmt, 9);

        transaction->previous_quantity =
            sqlite3_column_int(stmt, 10);

        transaction->new_quantity =
            sqlite3_column_int(stmt, 11);

        transaction->destination_location_id =
            sqlite3_column_int(stmt, 12);

        snprintf(
            transaction->destination_location_name,
            sizeof(transaction->destination_location_name),
            "%s",
            destination_location_name != NULL
                ? (const char *)destination_location_name
                : ""
        );

        snprintf(
            transaction->created_at,
            sizeof(transaction->created_at),
            "%s",
            created_at != NULL ? (const char *)created_at : ""
        );

        count++;
    }

    if (rc != SQLITE_DONE && count < max_transactions)
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

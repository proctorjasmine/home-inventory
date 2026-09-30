#include <stdio.h>
#include <string.h>
#include "product.h"

int product_find_by_barcode(Database *db, const char *barcode, ProductPackage *result){
    const char *sql =
        "SELECT "
        "p.id, "
        "pp.id, "
        "pp.barcode, "
        "p.name, "
        "p.brand, "
        "p.inventory_unit, "
        "pp.package_quantity "
        "FROM product_packages pp "
        "JOIN products p ON pp.product_id = p.id "
        "WHERE pp.barcode = ?;";

    sqlite3_stmt *stmt = NULL;

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
                "Failed to prepare barcode lookup: %s\n",
                sqlite3_errmsg(db->connection));

        return -1;
    }

    sqlite3_bind_text(
        stmt,
        1,
        barcode,
        -1,
        SQLITE_TRANSIENT
    );

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        result->product_id =
            sqlite3_column_int(stmt, 0);

        result->package_id =
            sqlite3_column_int(stmt, 1);

        snprintf(
            result->barcode,
            sizeof(result->barcode),
            "%s",
            sqlite3_column_text(stmt, 2)
        );

        snprintf(
            result->product_name,
            sizeof(result->product_name),
            "%s",
            sqlite3_column_text(stmt, 3)
        );

        snprintf(
            result->brand,
            sizeof(result->brand),
            "%s",
            sqlite3_column_text(stmt, 4)
        );

        snprintf(
            result->inventory_unit,
            sizeof(result->inventory_unit),
            "%s",
            sqlite3_column_text(stmt, 5)
        );

        result->package_quantity =
            sqlite3_column_int(stmt, 6);

        sqlite3_finalize(stmt);

        return 0;
    }

    if (rc == SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return 1;
    }

    fprintf(stderr,
            "Barcode lookup failed: %s\n",
            sqlite3_errmsg(db->connection));

    sqlite3_finalize(stmt);

    return -1;
}
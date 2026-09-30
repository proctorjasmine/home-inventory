#include <stdio.h>
#include "database.h"
#include "product.h"
#include "inventory.h"

int main(void){
    Database db = {0};

    if (database_open(&db, "data/inventory.db") != 0){
        return 1;
    }

    ProductPackage package;

    int rc = product_find_by_barcode(
        &db,
        "TEST-COKE-12",
        &package
    );

    if (rc == 0)
    {
        printf("Scanned: %s\n", package.product_name);
        printf("Package contains: %d %s(s)\n",
               package.package_quantity,
               package.inventory_unit);

        // Test data:
        // user_id     1 = Jasmine
        // location_id 4 = Refrigerator
        if (inventory_add(
                &db,
                package.product_id,
                4,
                1,
                package.package_quantity
            ) == 0)
        {
            printf(
                "Added %d %s(s) to inventory!\n",
                package.package_quantity,
                package.inventory_unit
            );
        }
    }
    else if (rc == 1)
    {
        printf("Unknown barcode.\n");
    }
    else
    {
        printf("Database error.\n");
    }

    database_close(&db);

    return 0;
}
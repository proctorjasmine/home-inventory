#include <stdio.h>
#include "database.h"
#include "product.h"

int main(void){
    Database db = {0};

    if (database_open(&db, "data/inventory.db") != 0){
        return 1;
    }

    ProductPackage package;

    int rc = product_find_by_barcode(
        &db,
        "DOES-NOT-EXIST",
        &package
    );

    if (rc == 0)
    {
        printf("Product found!\n\n");

        printf("Name: %s\n", package.product_name);
        printf("Brand: %s\n", package.brand);
        printf("Barcode: %s\n", package.barcode);
        printf("Package quantity: %d\n",
               package.package_quantity);
        printf("Inventory unit: %s\n",
               package.inventory_unit);
    }
    else if (rc == 1)
    {
        printf("Barcode not found.\n");
    }
    else
    {
        printf("Database error.\n");
    }

    database_close(&db);

    return 0;
}
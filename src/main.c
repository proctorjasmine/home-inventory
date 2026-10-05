#include <stdio.h>

#include "database.h"
#include "inventory.h"

#define MAX_INVENTORY_ITEMS 100

int main(void)
{
    Database db = {0};

    if (database_open(&db, "data/inventory.db") != 0){
        return 1;
    }

    InventoryItem items[MAX_INVENTORY_ITEMS];
    int item_count = 0;

    if (inventory_list(
            &db,
            items,
            MAX_INVENTORY_ITEMS,
            &item_count
        ) != 0)
    {
        database_close(&db);
        return 1;
    }

    printf("Current Inventory\n");
    printf("=================\n\n");

    for (int i = 0; i < item_count; i++)
    {
        printf(
            "%-20s %-15s %d %s(s)\n",
            items[i].product_name,
            items[i].location_name,
            items[i].quantity,
            items[i].inventory_unit
        );
    }

    database_close(&db);

    return 0;
}
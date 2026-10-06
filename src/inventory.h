#ifndef INVENTORY_H
#define INVENTORY_H

#include "database.h"

#define INVENTORY_NAME_MAX 128
#define INVENTORY_UNIT_MAX 32
#define LOCATION_NAME_MAX 128
#define USER_NAME_MAX 128
#define TIMESTAMP_MAX 32


typedef struct
{
    int product_id;
    int location_id;

    char product_name[INVENTORY_NAME_MAX];
    char inventory_unit[INVENTORY_UNIT_MAX];
    char location_name[LOCATION_NAME_MAX];

    int quantity;
} InventoryItem;

typedef struct
{
    int id;
    int product_id;
    int location_id;
    int user_id;

    char product_name[INVENTORY_NAME_MAX];
    char inventory_unit[INVENTORY_UNIT_MAX];
    char location_name[LOCATION_NAME_MAX];
    char user_name[USER_NAME_MAX];

    int quantity_change;

    char created_at[TIMESTAMP_MAX];
} InventoryTransaction;

int inventory_add(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    int quantity
);


int inventory_remove(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    int quantity
);

int inventory_get_quantity(
    Database *db,
    int product_id,
    int location_id,
    int *quantity
);

int inventory_list(
    Database *db,
    InventoryItem *items,
    int max_items,
    int *item_count
);

int inventory_transaction_list(
    Database *db,
    InventoryTransaction *transactions,
    int max_transactions,
    int *transaction_count
);

#endif
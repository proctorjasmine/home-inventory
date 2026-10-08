#ifndef INVENTORY_H
#define INVENTORY_H

#include "database.h"

#define INVENTORY_NAME_MAX 128
#define INVENTORY_UNIT_MAX 32
#define LOCATION_NAME_MAX 128
#define USER_NAME_MAX 128
#define TRANSACTION_TYPE_MAX 16
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

    char transaction_type[TRANSACTION_TYPE_MAX];

    int quantity_change;
    int previous_quantity;
    int new_quantity;

    int destination_location_id;
    char destination_location_name[LOCATION_NAME_MAX];

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

/*
 * Set inventory at one location to the physical count supplied by the user.
 *
 * Returns:
 *   0  success; inventory changed
 *   1  no change was necessary
 *  -1  database/validation error
 */
int inventory_adjust(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    int new_quantity
);

/*
 * Move inventory between two locations atomically.
 *
 * Returns:
 *   0  success
 *   1  not enough inventory at the source
 *  -1  database/validation error
 */
int inventory_move(
    Database *db,
    int product_id,
    int source_location_id,
    int destination_location_id,
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

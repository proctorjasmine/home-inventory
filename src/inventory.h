#ifndef INVENTORY_H
#define INVENTORY_H

#include "database.h"

int inventory_add(
    Database *db,
    int product_id,
    int location_id,
    int user_id,
    int quantity
);

#endif
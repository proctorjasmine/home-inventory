#ifndef PRODUCT_H
#define PRODUCT_H

#include "database.h"

#define PRODUCT_NAME_MAX 128
#define BRAND_MAX 128
#define UNIT_MAX 32
#define BARCODE_MAX 64

typedef struct
{
    int product_id;
    int package_id;

    char barcode[BARCODE_MAX];
    char product_name[PRODUCT_NAME_MAX];
    char brand[BRAND_MAX];
    char inventory_unit[UNIT_MAX];

    int package_quantity;
} ProductPackage;

int product_find_by_barcode(
    Database *db,
    const char *barcode,
    ProductPackage *result
);




#endif
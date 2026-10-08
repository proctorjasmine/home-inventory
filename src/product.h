#ifndef PRODUCT_H
#define PRODUCT_H

#include "database.h"

#define PRODUCT_NAME_MAX 128
#define BRAND_MAX 128
#define UNIT_MAX 32
#define BARCODE_MAX 64

typedef struct
{
    int id;

    char name[PRODUCT_NAME_MAX];
    char brand[BRAND_MAX];
    char inventory_unit[UNIT_MAX];

    int low_stock_threshold;
    int auto_add_grocery;
    int total_quantity;
} Product;

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

int product_create_with_package(
    Database *db,
    const char *name,
    const char *brand,
    const char *inventory_unit,
    int low_stock_threshold,
    int auto_add_grocery,
    const char *barcode,
    int package_quantity,
    int *product_id_out
);

int product_add_package(
    Database *db,
    int product_id,
    const char *barcode,
    int package_quantity
);

int product_list(
    Database *db,
    Product *products,
    int max_products,
    int *product_count
);

int product_update(
    Database *db,
    int product_id,
    const char *name,
    const char *brand,
    const char *inventory_unit,
    int low_stock_threshold,
    int auto_add_grocery
);

#endif
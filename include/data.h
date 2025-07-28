#ifndef DATA_H
#define DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    size_t size;
    size_t length;
    char *data;
} StringBuffer;

typedef struct {
    const char *text;
    const char *emoji;
    bool discovery;
} ItemData;

typedef struct {
    size_t id;
    char *text;
    char *emoji;
    bool discovery;
} Item;

typedef struct {
    size_t size;
    size_t length;
    Item *list;
} Items;

typedef struct {
    size_t size;
    size_t count;
    float load_factor;
    uint8_t bits;
    Item **data;
} ItemMap;

typedef struct {
    size_t first;
    size_t second;
} RecipeData;

typedef struct {
    size_t first;
    size_t second;
    size_t result;
} Recipe;

typedef struct {
    size_t size;
    size_t length;
    Recipe *list;
} Recipes;

typedef struct {
    size_t size;
    size_t count;
    float load_factor;
    uint8_t bits;
    Recipe **data;
} RecipeMap;

typedef struct {
    StringBuffer strings;
    Items item_arr;
    Recipes recipe_arr;
    ItemMap item_map;
    RecipeMap recipe_map;
} Data;

typedef struct {
    Item *item;
    Recipe *recipe;
} DataResult;

void Data_init(Data *data, size_t reserve_items, size_t reserve_recipes, size_t reserve_strings);

void Data_free(Data *data);

Item *Data_item_get(Data *data, const char *text);

Recipe *Data_recipe_get(Data *data, size_t first, size_t second);

Item *Data_item_insert(Data *data, ItemData item_dat);

Recipe *Data_recipe_insert(Data *data, RecipeData recipe_dat, size_t result);

DataResult Data_insert(Data *data, ItemData item_dat, RecipeData recipe_dat);

#endif

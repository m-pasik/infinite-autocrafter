#ifndef DATA_H
#define DATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct StringChunk {
    size_t used;
    char *data;
    struct StringChunk *next;
} StringChunk;

typedef struct StringBuffer {
    StringChunk *head;
    size_t chunk_size;
} StringBuffer;

typedef struct ItemData {
    const char *text;
    const char *emoji;
    bool discovery;
} ItemData;

typedef struct Item {
    size_t id;
    const char *text;
    const char *emoji;
    bool discovery;
} Item;

typedef struct Items {
    size_t size;
    size_t length;
    Item *list;
} Items;

typedef struct ItemMap {
    size_t size;
    size_t count;
    float load_factor;
    uint8_t bits;
    Item **data;
} ItemMap;

typedef struct RecipeData {
    size_t first;
    size_t second;
} RecipeData;

typedef struct Recipe {
    size_t first;
    size_t second;
    size_t result;
} Recipe;

typedef struct Recipes {
    size_t size;
    size_t length;
    Recipe *list;
} Recipes;

typedef struct RecipeMap {
    size_t size;
    size_t count;
    float load_factor;
    uint8_t bits;
    Recipe **recipes;
    Recipe **results;
} RecipeMap;

typedef struct Data {
    StringBuffer strings;
    Items item_arr;
    Recipes recipe_arr;
    ItemMap item_map;
    RecipeMap recipe_map;
} Data;

typedef struct DataResult {
    Item *item;
    Recipe *recipe;
} DataResult;

void Data_init(Data *data, size_t reserve_items, size_t reserve_recipes, size_t string_chunk_size);

void Data_free(Data *data);

Item *Data_item_get(Data *data, const char *text);

bool Data_has_recipe(Data *data, size_t id);

Recipe *Data_recipe_get(Data *data, size_t first, size_t second);

Item *Data_item_insert(Data *data, ItemData item_dat);

Recipe *Data_recipe_insert(Data *data, RecipeData recipe_dat, size_t result);

DataResult Data_insert(Data *data, ItemData item_dat, RecipeData recipe_dat);

#endif

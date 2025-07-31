#include "data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Hash table load factor
#define LOAD_FACTOR 0.75

// 64-bit FNV-1a constants
#define FNV_OFFSET_BASIS 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

static uint64_t fnv1a_hash(const char *data)
{
    uint64_t hash = FNV_OFFSET_BASIS;

    // Compute FNV-1a hash
    while (*data) {
        hash ^= (uint8_t)(*data);
        hash *= FNV_PRIME;
        data++;
    }

    return hash;
}

static size_t hash_combine(size_t a, size_t b)
{
    return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
}

static size_t hash_string(const char *data, uint8_t bits)
{
    uint64_t hash = fnv1a_hash(data);

    // Mask to fit the table size
    size_t mask = ((size_t)1 << bits) - 1;
    return (size_t)(hash & mask);
}

static size_t hash_ids(size_t a, size_t b, uint8_t bits)
{
    size_t hash = hash_combine(a, b);

    // Mask to fit the table size
    size_t mask = ((size_t)1 << bits) - 1;
    return hash & mask;
}

static void calc_size(size_t min_size, float load_factor, size_t *size, uint8_t *bits)
{
    *size = 16;
    *bits = 4;
    while ((float)min_size / *size > load_factor) {
        *size *= 2;
        (*bits)++;
    }
}

static StringChunk *StringChunk_create(size_t chunk_size)
{
    StringChunk *chunk = malloc(sizeof(StringChunk));
    chunk->data = malloc(chunk_size);
    chunk->used = 0;
    chunk->next = NULL;

    return chunk;
}

void Data_init(Data *data, size_t reserve_items, size_t reserve_recipes, size_t string_chunk_size)
{
    // Initialize dynamic arrays
    {
        // Initialize the string buffer
        data->strings.chunk_size = string_chunk_size;
        data->strings.head = StringChunk_create(string_chunk_size);

        // Initialize the item array
        data->item_arr.size = reserve_items;
        data->item_arr.length = 0;
        data->item_arr.list = malloc(reserve_items * sizeof(Item));

        // Initialize the recipe array
        data->recipe_arr.size = reserve_recipes;
        data->recipe_arr.length = 0;
        data->recipe_arr.list = malloc(reserve_recipes * sizeof(Recipe));
    }

    // Initialize hash tables
    {
        size_t map_size;
        uint8_t map_bits;

        // Initialize the item hash table
        calc_size(reserve_items, LOAD_FACTOR, &map_size, &map_bits);
        data->item_map.size = map_size;
        data->item_map.count = 0;
        data->item_map.load_factor = LOAD_FACTOR;
        data->item_map.bits = map_bits;
        data->item_map.data = calloc(map_size, sizeof(Item *));

        // Initialize the recipe hash table
        calc_size(reserve_recipes, LOAD_FACTOR, &map_size, &map_bits);
        data->recipe_map.size = map_size;
        data->recipe_map.count = 0;
        data->recipe_map.load_factor = LOAD_FACTOR;
        data->recipe_map.bits = map_bits;
        data->recipe_map.data = calloc(map_size, sizeof(Recipe *));
    }
}

void Data_free(Data *data)
{
    for (StringChunk *curr = data->strings.head, *next; curr; curr = next) {
        next = curr->next;
        free(curr->data);
        free(curr);
    }
    free(data->item_arr.list);
    free(data->recipe_arr.list);
    free(data->item_map.data);
    free(data->recipe_map.data);
}

Item *Data_item_get(Data *data, const char *text)
{
    const size_t hash = hash_string(text, data->item_map.bits);
    for (size_t i = hash; data->item_map.data[i]; i = (i + 1) & (data->item_map.size - 1))
        if (!strcmp(data->item_map.data[i]->text, text))
            return data->item_map.data[i];
    return NULL;
}

Recipe *Data_recipe_get(Data *data, size_t first, size_t second)
{
    const size_t hash = hash_ids(first, second, data->recipe_map.bits);
    for (size_t i = hash; data->recipe_map.data[i]; i = (i + 1) & (data->recipe_map.size - 1))
        if (data->recipe_map.data[i]->first == first && data->recipe_map.data[i]->second == second)
            return data->recipe_map.data[i];
    return NULL;
}

static void ItemMap_insert_internal(ItemMap *item_map, Item *item)
{
    const size_t item_hash = hash_string(item->text, item_map->bits);
    size_t i;
    for (i = item_hash; item_map->data[i]; i = (i + 1) & (item_map->size - 1)) continue;
    item_map->data[i] = item;
    item_map->count++;
}

static void ItemMap_rehash(ItemMap *item_map, Items *items)
{
    memset(item_map->data, 0, item_map->size * sizeof(Item *));
    item_map->count = 0;
    for (size_t i = 0; i < items->length; i++)
        ItemMap_insert_internal(item_map, items->list + i);
}

static void ItemMap_insert(ItemMap *item_map, Items *items, Item *item)
{
    if ((float)item_map->count / item_map->size > item_map->load_factor) {
        item_map->size *= 2;
        item_map->bits++;
        item_map->data = realloc(item_map->data, item_map->size * sizeof(Item *));
        ItemMap_rehash(item_map, items);
    } else {
        ItemMap_insert_internal(item_map, item);
    }
}

static void RecipeMap_insert_internal(RecipeMap *recipe_map, Recipe *recipe)
{
    const size_t recipe_hash = hash_ids(recipe->first, recipe->second, recipe_map->bits);
    size_t i;
    for (i = recipe_hash; recipe_map->data[i]; i = (i + 1) & (recipe_map->size - 1)) continue;
    recipe_map->data[i] = recipe;
    recipe_map->count++;
}

static void RecipeMap_rehash(RecipeMap *recipe_map, Recipes *recipes)
{
    memset(recipe_map->data, 0, recipe_map->size * sizeof(Recipe *));
    recipe_map->count = 0;
    for (size_t i = 0; i < recipes->length; i++)
        RecipeMap_insert_internal(recipe_map, recipes->list + i);
}

static void RecipeMap_insert(RecipeMap *recipe_map, Recipes *recipes, Recipe *recipe)
{
    if ((float)recipe_map->count / recipe_map->size > recipe_map->load_factor) {
        recipe_map->size *= 2;
        recipe_map->bits++;
        recipe_map->data = realloc(recipe_map->data, recipe_map->size * sizeof(Recipe *));
        RecipeMap_rehash(recipe_map, recipes);
    } else {
        RecipeMap_insert_internal(recipe_map, recipe);
    }
}

static const char *StringBuffer_insert(StringBuffer *strings, const char *text)
{
    size_t len = strlen(text) + 1;

    if (len > strings->chunk_size)
        return NULL;

    if (strings->chunk_size - strings->head->used < len) {
        StringChunk *new_chunk = StringChunk_create(strings->chunk_size);

        new_chunk->next = strings->head;
        strings->head = new_chunk;
    }

    char *string = strings->head->data + strings->head->used;
    memcpy(string, text, len);
    strings->head->used += len;

    return string;
}

static Item *Items_insert(Data *data, ItemData item_dat)
{
    Items *items = &data->item_arr;
    ItemMap *item_map = &data->item_map;
    StringBuffer *strings = &data->strings;

    if (items->length + 1 > items->size) {
        items->size *= 2;
        items->list = realloc(items->list, items->size * sizeof(Item));
        ItemMap_rehash(item_map, items);
    }

    Item *item = items->list + items->length;
    item->id = items->length;
    item->text = StringBuffer_insert(strings, item_dat.text);
    item->emoji = StringBuffer_insert(strings, item_dat.emoji);
    item->discovery = item_dat.discovery;
    items->length++;

    return item;
}

static Recipe *Recipes_insert(Data *data, RecipeData recipe_dat, size_t result)
{
    Recipes *recipes = &data->recipe_arr;
    RecipeMap *recipe_map = &data->recipe_map;

    if (recipes->length + 1 > recipes->size) {
        recipes->size *= 2;
        recipes->list = realloc(recipes->list, recipes->size * sizeof(Recipe));
        RecipeMap_rehash(recipe_map, recipes);
    }

    Recipe *recipe = recipes->list + recipes->length;
    recipe->first = recipe_dat.first;
    recipe->second = recipe_dat.second;
    recipe->result = result;
    recipes->length++;

    return recipe;
}

Item *Data_item_insert(Data *data, ItemData item_dat)
{
    Item *item = NULL;
    bool item_exists = false;

    const size_t item_hash = hash_string(item_dat.text, data->item_map.bits);
    size_t i;
    for (i = item_hash; data->item_map.data[i]; i = (i + 1) & (data->item_map.size - 1)) {
        if (!strcmp(data->item_map.data[i]->text, item_dat.text)) {
            item = data->item_map.data[i];
            item_exists = true;
            break;
        }
    }
    if (!item_exists) {
        item = Items_insert(data, item_dat);
        ItemMap_insert(&data->item_map, &data->item_arr, item);
    }

    return item;
}

Recipe *Data_recipe_insert(Data *data, RecipeData recipe_dat, size_t result)
{
    Recipe *recipe = NULL;
    bool recipe_exists = false;

    const size_t recipe_hash = hash_ids(recipe_dat.first, recipe_dat.second, data->recipe_map.bits);
    size_t i;
    for (i = recipe_hash; data->recipe_map.data[i]; i = (i + 1) & (data->recipe_map.size - 1)) {
        if (data->recipe_map.data[i]->first == recipe_dat.first && data->recipe_map.data[i]->second == recipe_dat.second) {
            recipe = data->recipe_map.data[i];
            recipe_exists = true;
            break;
        }
    }
    if (!recipe_exists) {
        recipe = Recipes_insert(data, recipe_dat, result);
        RecipeMap_insert(&data->recipe_map, &data->recipe_arr, recipe);
    }

    return recipe;
}

DataResult Data_insert(Data *data, ItemData item_dat, RecipeData recipe_dat)
{
    // Check if item exists and insert
    Item *item;
    bool item_exists = false;

    const size_t item_hash = hash_string(item_dat.text, data->item_map.bits);
    size_t i;
    for (i = item_hash; data->item_map.data[i]; i = (i + 1) & (data->item_map.size - 1)) {
        if (!strcmp(data->item_map.data[i]->text, item_dat.text)) {
            item = data->item_map.data[i];
            item_exists = true;
            break;
        }
    }
    if (!item_exists) {
        item = Items_insert(data, item_dat);
        ItemMap_insert(&data->item_map, &data->item_arr, item);
    }

    // Insert recipe
    Recipe *recipe = Recipes_insert(data, recipe_dat, item->id);
    RecipeMap_insert(&data->recipe_map, &data->recipe_arr, recipe);

    DataResult dat = { item, recipe };

    return dat;
}

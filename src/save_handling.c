#include "save_handling.h"
#include "data.h"

#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

#define CHUNK 16384
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static char *load_gzip(const char *filename, size_t *out_size)
{
    gzFile file = gzopen(filename, "rb");

    size_t size = CHUNK;
    char *buffer = malloc(size);

    size_t length = 0;
    int read;
    unsigned char tmp[CHUNK];

    while ((read = gzread(file, tmp, CHUNK)) > 0) {
        if (length + read > size) {
            size *= 2;
            buffer = realloc(buffer, size);
        }
        memcpy(buffer + length, tmp, read);
        length += read;
    }

    gzclose(file);
    buffer[length] = '\0';
    buffer = realloc(buffer, length + 1);

    *out_size = length;
    return buffer;
}

Save *load_save(const char *filename)
{
    Save *save = malloc(sizeof(Save));
    Data *data = malloc(sizeof(Data));
    save->data = data;

    size_t size;
    char *decompressed = load_gzip(filename, &size);

    json_object *parsed_json = json_tokener_parse(decompressed);

    json_object *obj;

    if (json_object_object_get_ex(parsed_json, "name", &obj)) {
        save->name = malloc(json_object_get_string_len(obj) + 1);
        strcpy(save->name, json_object_get_string(obj));
    } else {
        const char unnamed_str[] = "Unnamed";
        save->name = malloc(sizeof(unnamed_str));
        strcpy(save->name, unnamed_str);
    }

    if (!json_object_object_get_ex(parsed_json, "version", &obj)
        || sscanf(json_object_get_string(obj), "%d.%d", &save->version_major, &save->version_minor) != 2) {
        save->version_major = 1;
        save->version_minor = 0;
    }

    if (json_object_object_get_ex(parsed_json, "created", &obj))
        save->created = json_object_get_int64(obj);
    else
        save->created = time(NULL);

    if (json_object_object_get_ex(parsed_json, "updated", &obj))
        save->updated = json_object_get_int64(obj);
    else
        save->updated = time(NULL);

    json_object *items_obj;

    if (!json_object_object_get_ex(parsed_json, "items", &items_obj)) {
    }

    size_t itemc = json_object_array_length(items_obj);

    Data_init(data, MAX(512, itemc), MAX(2048, itemc * 4), 16384);

    for (size_t i = 0; i < itemc; i++) {
        json_object *item_obj = json_object_array_get_idx(items_obj, i);

        ItemData item_dat;
        if (json_object_object_get_ex(item_obj, "text", &obj))
            item_dat.text = json_object_get_string(obj);
        else
            continue;

        if (json_object_object_get_ex(item_obj, "emoji", &obj))
            item_dat.emoji = json_object_get_string(obj);
        else
            item_dat.emoji = "";

        if (json_object_object_get_ex(item_obj, "discovery", &obj) && json_object_get_boolean(obj))
            item_dat.discovery = true;
        else
            item_dat.discovery = false;

        Item *item = Data_item_insert(data, item_dat);

        json_object *recipes_obj;
        if (!json_object_object_get_ex(item_obj, "recipes", &recipes_obj))
            continue;

        size_t recipec = json_object_array_length(recipes_obj);
        for (size_t j = 0; j < recipec; j++) {
            json_object *recipe_obj = json_object_array_get_idx(recipes_obj, j);

            json_object *first_obj = json_object_array_get_idx(recipe_obj, 0),
                        *second_obj = json_object_array_get_idx(recipe_obj, 1);

            if (!first_obj || !second_obj)
                continue;

            RecipeData recipe_dat;

            recipe_dat.first = json_object_get_uint64(first_obj);
            recipe_dat.second = json_object_get_uint64(second_obj);

            Data_recipe_insert(data, recipe_dat, item->id);
        }
    }

    free(decompressed);
    json_object_put(parsed_json);

    return save;
}

void export_save(Save *save, const char *filename)
{
    Data *data = save->data;
    Items *items = &data->item_arr;
    Recipes *recipes = &data->recipe_arr;

    json_object *root_obj = json_object_new_object(),
                *items_obj = json_object_new_array(),
                **item_obj_arr = malloc(items->length * sizeof(json_object *));

    char version[32];
    snprintf(version, sizeof(version), "%d.%d", save->version_major, save->version_minor);

    json_object_object_add(root_obj, "name", json_object_new_string(save->name));
    json_object_object_add(root_obj, "version", json_object_new_string(version));
    json_object_object_add(root_obj, "created", json_object_new_int64(save->created));
    json_object_object_add(root_obj, "updated", json_object_new_int64(time(NULL)));
    json_object_object_add(root_obj, "instances", json_object_new_array());
    json_object_object_add(root_obj, "items", items_obj);

    for (size_t i = 0; i < items->length; i++) {
        Item *item = &items->list[i];

        json_object *item_obj = json_object_new_object();
        json_object_object_add(item_obj, "id", json_object_new_uint64(item->id));
        json_object_object_add(item_obj, "text", json_object_new_string(item->text));
        json_object_object_add(item_obj, "emoji", json_object_new_string(item->emoji));
        if (item->discovery)
            json_object_object_add(item_obj, "discovery", json_object_new_boolean(true));

        item_obj_arr[i] = item_obj;
        json_object_array_add(items_obj, item_obj);
    }

    for (size_t i = 0; i < recipes->length; i++) {
        Recipe *recipe = &recipes->list[i];

        json_object *recipe_obj = json_object_new_array();
        json_object_array_add(recipe_obj, json_object_new_uint64(recipe->first));
        json_object_array_add(recipe_obj, json_object_new_uint64(recipe->second));

        json_object *item_obj = item_obj_arr[recipe->result];
        json_object *recipes_obj;
        if (!json_object_object_get_ex(item_obj, "recipes", &recipes_obj))
            json_object_object_add(item_obj, "recipes", (recipes_obj = json_object_new_array()));

        json_object_array_add(recipes_obj, recipe_obj);
    }

    const char *json_str = json_object_to_json_string_ext(root_obj, JSON_C_TO_STRING_PLAIN);

    size_t json_len = strlen(json_str);
    gzFile file = gzopen(filename, "wb");
    int written = gzwrite(file, json_str, json_len);

    if (written == 0) {
        int err;
        const char *msg = gzerror(file, &err);
        fprintf(stderr, "json string: %s\nlength: %zu\nerror message: %s\nerror code: %d\n", json_str, json_len, msg, err);
    }

    gzclose(file);
    json_object_put(root_obj);
    free(item_obj_arr);
}

void free_save(Save *save)
{
    free(save->name);
    Data_free(save->data);
    free(save->data);
    free(save);
}

void save_rename_old(const char *filename)
{
    size_t size = strlen(filename) + 5;
    char *dest = malloc(size);
    snprintf(dest, size, "%s.old", filename);
    rename(filename, dest);
    free(dest);
}

#include "fix.h"
#include "data.h"
#include "net_utils.h"

#include <curl/curl.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <json-c/json_tokener.h>
#include <json-c/json_types.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

bool find_recipe(Data *data, Item *item)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    size_t offset = 0, total = 0;

    bool found = false;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    char format[] = "https://infinibrowser.wiki/api/recipes?id=%s&offset=%zu";
    char *name_escaped = curl_easy_escape(curl, item->text, 0);

    size_t url_size = sizeof(format) + strlen(name_escaped) + 20;

    char *url = malloc(url_size);

    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        goto cleanup;
    }

    char *response = malloc(1);
    response[0] = '\0';

    headers = curl_slist_append(headers, "User-Agent: Hello, World!");
    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.5");
    headers = curl_slist_append(headers, "Alt-Used: infinibrowser.wiki");
    headers = curl_slist_append(headers, "Sec-Fetch-Dest: empty");
    headers = curl_slist_append(headers, "Sec-Fetch-Mode: cors");
    headers = curl_slist_append(headers, "Sec-Fetch-Site: same-origin");
    headers = curl_slist_append(headers, "Sec-GPC: 1");
    headers = curl_slist_append(headers, "Priority: u=4");

    curl_easy_setopt(curl, CURLOPT_REFERER, "https://infinibrowser.wiki/");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    do {
        snprintf(url, url_size, format, name_escaped, offset);

        curl_easy_setopt(curl, CURLOPT_URL, url);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(res));
            break;
        }

        json_object *parsed_json = json_tokener_parse(response);

        json_object *total_obj, *recipes_obj;

        if (!json_object_object_get_ex(parsed_json, "total", &total_obj)
            || !json_object_object_get_ex(parsed_json, "recipes", &recipes_obj)) {
            json_object_put(parsed_json);
            break;
        }

        total = json_object_get_uint64(total_obj);

        size_t recipes_length = json_object_array_length(recipes_obj);

        for (size_t i = 0; i < recipes_length; i++) {
            json_object *recipe_obj = json_object_array_get_idx(recipes_obj, i);

            json_object *first_obj = json_object_array_get_idx(recipe_obj, 0),
                        *second_obj = json_object_array_get_idx(recipe_obj, 1);

            json_object *first_obj_id, *second_obj_id;

            json_object_object_get_ex(first_obj, "id", &first_obj_id);
            json_object_object_get_ex(second_obj, "id", &second_obj_id);

            const char *first_name = json_object_get_string(first_obj_id),
                       *second_name = json_object_get_string(second_obj_id);

            Item *first = Data_item_get(data, first_name),
                 *second = Data_item_get(data, second_name);

            if (first && second) {
                RecipeData recipe_dat = { first->id, second->id };
                Data_recipe_insert(data, recipe_dat, item->id);
                found = true;
                printf("%s + %s = %s\n", first_name, second_name, item->text);
                break;
            }
        }

        offset += recipes_length;
        response[0] = '\0';

        json_object_put(parsed_json);

        usleep(100000);
    } while (!found && offset < total);

    free(response);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
cleanup:
    curl_global_cleanup();
    free(url);
    curl_free(name_escaped);

    return found;
}

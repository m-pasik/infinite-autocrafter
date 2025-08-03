#include "pair.h"

#include "net_utils.h"

#include <curl/curl.h>
#include <json-c/json.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

ResultGET *ic_get_result(const char *first, const char *second)
{
    CURL *curl;
    CURLcode res;
    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    char format[] = "https://neal.fun/api/infinite-craft/pair?first=%s&second=%s";
    char *first_escaped = curl_easy_escape(curl, first, 0),
         *second_escaped = curl_easy_escape(curl, second, 0);

    size_t size = sizeof(format) + strlen(first_escaped) + strlen(second_escaped);

    char *url = malloc(size);
    snprintf(url, size, format, first_escaped, second_escaped);

    ResultGET *pair_res = NULL;

    char *response = malloc(1);
    response[0] = '\0';

    if (!curl) {
        fprintf(stderr, "Failed to initialize CURL\n");
        pair_res = NULL;
        goto cleanup;
    }

    headers = curl_slist_append(headers, "User-Agent: Hi, Neal!");
    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.5");
    headers = curl_slist_append(headers, "Alt-Used: neal.fun");
    headers = curl_slist_append(headers, "Connection: keep_alive");
    headers = curl_slist_append(headers, "Sec-Fetch-Dest: empty");
    headers = curl_slist_append(headers, "Sec-Fetch-Mode: cors");
    headers = curl_slist_append(headers, "Sec-Fetch-Site: same-origin");
    headers = curl_slist_append(headers, "Sec-GPC: 1");
    headers = curl_slist_append(headers, "Priority: u=0");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_REFERER, "https://neal.fun/infinite-craft/");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "Request failed: %s\n", curl_easy_strerror(res));
        free(pair_res);
        pair_res = NULL;
        goto cleanup;
    }

    pair_res = malloc(sizeof(ResultGET));

    json_object *parsed_json = json_tokener_parse(response);

    json_object *obj;

    if (json_object_object_get_ex(parsed_json, "result", &obj)) {
        pair_res->result = malloc(json_object_get_string_len(obj) + 1);
        strcpy(pair_res->result, json_object_get_string(obj));
    } else {
        free(pair_res);
        pair_res = NULL;
        goto cleanup;
    }
    if (json_object_object_get_ex(parsed_json, "emoji", &obj)) {
        pair_res->emoji = malloc(json_object_get_string_len(obj) + 1);
        strcpy(pair_res->emoji, json_object_get_string(obj));
    }
    if (json_object_object_get_ex(parsed_json, "isNew", &obj)) {
        pair_res->is_new = json_object_get_boolean(obj);
    }

    json_object_put(parsed_json);
cleanup:
    curl_global_cleanup();
    free(response);
    free(url);
    curl_free(first_escaped);
    curl_free(second_escaped);

    return pair_res;
}

void ic_free(ResultGET *pair_res)
{
    free(pair_res->result);
    free(pair_res->emoji);
    free(pair_res);
}

ResultPair ic_new_pair(Data *data, struct xoshiro256pp_state *state)
{
    size_t a, b;
    ItemData item_dat;
    ResultGET *result;
    Item *first, *second;
    Items *items = &data->item_arr;

    do {
        a = xoshiro256pp(state) % items->length;
        b = xoshiro256pp(state) % items->length;
    } while (Data_recipe_get(data, a, b) || Data_recipe_get(data, b, a));

    RecipeData recipe_dat = { a, b };

    first = &items->list[a];
    second = &items->list[b];

    while (!(result = ic_get_result(first->text, second->text)))
        usleep(1000000);

    item_dat.text = result->result;
    item_dat.emoji = result->emoji;
    item_dat.discovery = result->is_new;

    ResultPair pair_res = { first, second, NULL };

    if (strcmp(result->result, "Nothing")) {
        DataResult dat = Data_insert(data, item_dat, recipe_dat);
        pair_res.first = &items->list[a];
        pair_res.second = &items->list[b];
        pair_res.result = dat.item;
    } else {
        pair_res.result = NULL;
    }

    ic_free(result);

    return pair_res;
}

#ifndef PAIR_H
#define PAIR_H

#include "data.h"
#include "random.h"
#include <stdbool.h>

typedef struct {
    char *result;
    char *emoji;
    bool is_new;
} ResultGET;

typedef struct {
    Item *first;
    Item *second;
    Item *result;
} ResultPair;

ResultGET *ic_get_result(const char *first, const char *second);

void ic_free(ResultGET *pair_res);

ResultPair ic_new_pair(Data *data, struct xoshiro256pp_state *state);

#endif

#include "data.h"
#include "fix.h"
#include "pair.h"
#include "random.h"
#include "save_handling.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Save *save;
const char *filename;

void handle_sigint(int sig)
{
    save_rename_old(filename);
    export_save(save, filename);
    free_save(save);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    filename = argv[2];

    save = load_save(filename);

    signal(SIGINT, handle_sigint);

    if (!strcmp(argv[1], "combine")) {
        struct xoshiro256pp_state rand_state;
        xoshiro256pp_seed(&rand_state);

        uint64_t count = strtoull(argv[3], NULL, 10);

        for (size_t i = 0; !count || i < count; i++) {
            ResultPair result = ic_new_pair(save->data, &rand_state);
            printf("%s + %s = %s\n", result.first->text, result.second->text, result.result ? result.result->text : "Nothing");
            usleep(100000);
        }
    } else if (!strcmp(argv[1], "fix")) {
        Items *items = &save->data->item_arr;
        for (size_t i = 4; i < items->length; i++)
            if (!Data_has_recipe(save->data, items->list[i].id))
                find_recipe(save->data, &items->list[i]);
    }

    save_rename_old(filename);
    export_save(save, filename);
    free_save(save);

    return EXIT_SUCCESS;
}

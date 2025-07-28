#include "data.h"
#include "pair.h"
#include "random.h"
#include "save_handling.h"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
    struct xoshiro256pp_state rand_state;
    xoshiro256pp_seed(&rand_state);

    filename = argv[1];

    save = load_save(filename);
    uint64_t count = strtoull(argv[2], NULL, 10);

    signal(SIGINT, handle_sigint);

    for (size_t i = 0; !count || i < count; i++) {
        ResultPair result = ic_new_pair(save->data, &rand_state);
        printf("%s + %s = %s\n", result.first->text, result.second->text, result.result ? result.result->text : "Nothing");
        usleep(100000);
    }

    save_rename_old(filename);
    export_save(save, filename);
    free_save(save);

    return EXIT_SUCCESS;
}

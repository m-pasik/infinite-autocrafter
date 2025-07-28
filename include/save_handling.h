#ifndef SAVE_H
#define SAVE_H

#include "data.h"
#include <stdint.h>
#include <time.h>

typedef struct {
    char *name;
    int version_major;
    int version_minor;
    int64_t created;
    int64_t updated;
    Data *data;
} Save;

Save *load_save(const char *filename);

void export_save(Save *save, const char *filename);

void free_save(Save *save);

void save_rename_old(const char *filename);

#endif

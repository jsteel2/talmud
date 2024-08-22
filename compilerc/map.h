#ifndef __MAP_H
#define __MAP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    char *key;
    uint64_t value;
} MapEntry;

typedef struct
{
    MapEntry *entries;
    size_t len;
    size_t size;
} Map;

bool map_init(Map *m);
void map_free(Map *m);
bool map_get(Map *m, char *key, uint64_t *value);
bool map_set(Map *m, char *key, uint64_t value);

#endif
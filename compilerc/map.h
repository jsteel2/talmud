#ifndef __MAP_H
#define __MAP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    char *key;
    uint64_t value;
    bool is_label;
} MapEntry;

typedef struct
{
    MapEntry *entries;
    size_t len;
    size_t size;
} Map;

bool map_init(Map *m);
void map_free(Map *m);
void map_free2(Map *m);
bool map_get(Map *m, char *key, uint64_t *value);
bool map_get2(Map *m, char *key, uint64_t *value, bool *is_label);
bool map_set(Map *m, char *key, uint64_t value);
bool map_set2(Map *m, char *key, uint64_t value, bool is_label);

#endif
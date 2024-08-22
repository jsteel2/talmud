#include <stdlib.h>
#include <string.h>
#include "map.h"

// TODO: make this into a proper hashmap

bool map_init(Map *m)
{
    m->len = 0;
    m->size = 1024;
    return (m->entries = malloc(m->size * sizeof(MapEntry)));
}

void map_free(Map *m)
{
    for (size_t i = 0; i < m->len; i++)
    {
        free(m->entries[i].key);
    }
    free(m->entries);
}

bool map_get(Map *m, char *key, uint64_t *value)
{
    for (size_t i = 0; i < m->len; i++)
    {
        if (strcmp(key, m->entries[i].key) == 0)
        {
            *value = m->entries[i].value;
            return true;
        }
    }
    return false;
}

bool map_set(Map *m, char *key, uint64_t value)
{
    for (size_t i = 0; i < m->len; i++)
    {
        if (strcmp(key, m->entries[i].key) == 0)
        {
            m->entries[i].value = value;
            return true;
        }
    }

    m->entries[m->len++] = (MapEntry){key, value};
    if (m->len >= m->size)
    {
        m->size *= 2;
        m->entries = realloc(m->entries, m->size);
        if (!m->entries) return false;
    }
    return true;
}
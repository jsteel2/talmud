#include <stdlib.h>
#include <string.h>
#include "map.h"

uint32_t hash_key(char *key)
{
    uint32_t hash = 2166136261;
    for (char *p = key; *p; p++)
    {
        hash ^= *p;
        hash *= 16777619;
    }
    return hash;
}

bool map_init(Map *m)
{
    m->len = 0;
    m->size = 1024;
    return (m->entries = calloc(m->size, sizeof(MapEntry)));
}

void map_free2(Map *m)
{
    for (size_t i = 0; i < m->size; i++)
    {
        free(m->entries[i].key);
        m->entries[i].key = NULL;
    }
    m->len = 0;
}

void map_free(Map *m)
{
    map_free2(m);
    free(m->entries);
}

MapEntry *map_ref_entry(MapEntry *entries, size_t size, char *key, size_t *index)
{
    if (!key) return NULL;
    uint32_t hash = hash_key(key);
    size_t i = hash & (size - 1);

    while (entries[i].key)
    {
        if (strcmp(key, entries[i].key) == 0) return &entries[i];
        i++;
        if (i > size) i = 0;
    }

    if (index) *index = i;
    return NULL;
}

MapEntry *map_ref(Map *m, char *key)
{
    return map_ref_entry(m->entries, m->size, key, NULL);
}

bool map_get(Map *m, char *key, uint64_t *value)
{
    MapEntry *e = map_ref(m, key);
    if (!e) return false;
    *value = e->value;
    return true;
}

void map_set_entry(MapEntry *entries, size_t size, size_t *len, char *key, uint64_t value)
{
    size_t i;
    MapEntry *entry = map_ref_entry(entries, size, key, &i);

    if (entry)
    {
        entry->value = value;
    }
    else
    {
        if (len) (*len)++;
        entries[i].key = key;
        entries[i].value = value;
    }
}

bool map_expand(Map *m)
{
    size_t size = m->size * 2;
    MapEntry *entries = calloc(size, sizeof(MapEntry));
    if (!entries) return false;

    for (size_t i = 0; i < m->size; i++)
    {
        if (!m->entries[i].key) continue;
        map_set_entry(entries, size, NULL, m->entries[i].key, m->entries[i].value);
    }

    free(m->entries);
    m->entries = entries;
    m->size = size;
    return true;
}

bool map_set(Map *m, char *key, uint64_t value)
{
    if (m->len >= m->size / 2)
    {
        if (!map_expand(m)) return false;
    }

    map_set_entry(m->entries, m->size, &m->len, key, value);
    return true;
}
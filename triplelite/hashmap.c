#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

int hashmap_init(HashMap *map, size_t n_buckets)
{
    map->buckets = calloc(n_buckets, sizeof(HashEntry *));
    if (map->buckets == NULL) {
        return -1;
    }
    map->n_buckets = n_buckets;
    map->len = 0;
    return 0;
}

static size_t hashmap_hash(const char *key, size_t n_buckets)
{
    size_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % n_buckets;
}

int hashmap_get(HashMap *map, const char *key, size_t *out)
{
    size_t idx = hashmap_hash(key, map->n_buckets);
    HashEntry *entry = map->buckets[idx];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            *out = entry->value;
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

int hashmap_put(HashMap *map, const char *key, size_t value)
{
    size_t idx = hashmap_hash(key, map->n_buckets);
    HashEntry *entry = map->buckets[idx];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return 0;
        }
        entry = entry->next;
    }
    HashEntry *new_entry = malloc(sizeof(HashEntry));
    if (new_entry == NULL) {
        return -1;
    }
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = map->buckets[idx];
    map->buckets[idx] = new_entry;
    map->len++;
    return 0;
}

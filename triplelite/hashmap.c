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

size_t hash_string(const char *s)
{
    size_t hash = 5381;
    int c;
    while ((c = *s++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int hashmap_get(HashMap *map, const char *key, size_t *out)
{
    size_t bucket = hash_string(key) % map->n_buckets;
    HashEntry *entry = map->buckets[bucket];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            *out = entry->value;
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

static int hashmap_resize(HashMap *map)
{
    size_t new_n_buckets = map->n_buckets * 2;
    HashEntry **new_buckets = calloc(new_n_buckets, sizeof(HashEntry *));
    if (new_buckets == NULL) {
        return -1;
    }
    for (size_t i = 0; i < map->n_buckets; i++) {
        HashEntry *entry = map->buckets[i];
        while (entry != NULL) {
            HashEntry *next = entry->next;
            size_t bucket = hash_string(entry->key) % new_n_buckets;
            entry->next = new_buckets[bucket];
            new_buckets[bucket] = entry;
            entry = next;
        }
    }
    free(map->buckets);
    map->buckets = new_buckets;
    map->n_buckets = new_n_buckets;
    return 0;
}

int hashmap_put(HashMap *map, const char *key, size_t value)
{
    size_t bucket = hash_string(key) % map->n_buckets;
    HashEntry *entry = map->buckets[bucket];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return 0;
        }
        entry = entry->next;
    }
    if (map->len * 4 >= map->n_buckets * 3) {
        if (hashmap_resize(map) < 0) {
            return -1;
        }
        bucket = hash_string(key) % map->n_buckets;
    }
    HashEntry *new_entry = malloc(sizeof(HashEntry));
    if (new_entry == NULL) {
        return -1;
    }
    new_entry->key = strdup(key);
    if (new_entry->key == NULL) {
        free(new_entry);
        return -1;
    }
    new_entry->value = value;
    new_entry->next = map->buckets[bucket];
    map->buckets[bucket] = new_entry;
    map->len++;
    return 0;
}

void hashmap_free(HashMap *map)
{
    for (size_t i = 0; i < map->n_buckets; i++) {
        HashEntry *entry = map->buckets[i];
        while (entry != NULL) {
            HashEntry *next = entry->next;
            free((char *)entry->key);
            free(entry);
            entry = next;
        }
    }
    free(map->buckets);
}

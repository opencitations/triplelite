#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

typedef struct HashEntry {
    const char *key;
    size_t value;
    struct HashEntry *next;
} HashEntry;

typedef struct {
    HashEntry **buckets;
    size_t n_buckets;
    size_t len;
} HashMap;

size_t hash_string(const char *s);
int hashmap_init(HashMap *map, size_t n_buckets);
int hashmap_get(HashMap *map, const char *key, size_t *out);
int hashmap_put(HashMap *map, const char *key, size_t value);

#endif
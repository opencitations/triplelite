#include "rdfterm_hashmap.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

static size_t rdfterm_hash(const RDFTerm *term, size_t n_buckets)
{
    size_t h = hash_string(term->type);
    h = h * 31 + hash_string(term->value);
    h = h * 31 + hash_string(term->datatype);
    h = h * 31 + hash_string(term->lang);
    return h % n_buckets;
}

static int rdfterm_equal(const RDFTerm *a, const RDFTerm *b)
{
    return strcmp(a->type, b->type) == 0 &&
           strcmp(a->value, b->value) == 0 &&
           strcmp(a->datatype, b->datatype) == 0 &&
           strcmp(a->lang, b->lang) == 0;
}

int rdfterm_hashmap_init(RDFTermHashMap *map, size_t n_buckets)
{
    map->buckets = calloc(n_buckets, sizeof(RDFTermHashEntry *));
    if (map->buckets == NULL) {
        return -1;
    }
    map->n_buckets = n_buckets;
    map->len = 0;
    return 0;
}

int rdfterm_hashmap_get(RDFTermHashMap *map, const RDFTerm *key, size_t *out)
{
    size_t idx = rdfterm_hash(key, map->n_buckets);
    RDFTermHashEntry *entry = map->buckets[idx];
    while (entry != NULL) {
        if (rdfterm_equal(&entry->key, key)) {
            *out = entry->value;
            return 1;
        }
        entry = entry->next;
    }
    return 0;
}

int rdfterm_hashmap_put(RDFTermHashMap *map, const RDFTerm *key, size_t value)
{
    size_t idx = rdfterm_hash(key, map->n_buckets);
    RDFTermHashEntry *entry = map->buckets[idx];
    while (entry != NULL) {
        if (rdfterm_equal(&entry->key, key)) {
            entry->value = value;
            return 0;
        }
        entry = entry->next;
    }
    RDFTermHashEntry *new_entry = malloc(sizeof(RDFTermHashEntry));
    if (new_entry == NULL) {
        return -1;
    }
    new_entry->key = *key;
    new_entry->value = value;
    new_entry->next = map->buckets[idx];
    map->buckets[idx] = new_entry;
    map->len++;
    return 0;
}

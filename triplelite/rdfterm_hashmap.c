#include "rdfterm_hashmap.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

static size_t rdfterm_hash(const RDFTerm *term, size_t n_buckets)
{
    size_t hash = hash_string(term->type);
    hash = hash * 31 + hash_string(term->value);
    hash = hash * 31 + hash_string(term->datatype);
    hash = hash * 31 + hash_string(term->lang);
    return hash % n_buckets;
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
    size_t bucket = rdfterm_hash(key, map->n_buckets);
    RDFTermHashEntry *entry = map->buckets[bucket];
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
    size_t bucket = rdfterm_hash(key, map->n_buckets);
    RDFTermHashEntry *entry = map->buckets[bucket];
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
    if (rdfterm_copy(&new_entry->key, key) < 0) {
        free(new_entry);
        return -1;
    }
    new_entry->value = value;
    new_entry->next = map->buckets[bucket];
    map->buckets[bucket] = new_entry;
    map->len++;
    return 0;
}

void rdfterm_hashmap_free(RDFTermHashMap *map)
{
    for (size_t i = 0; i < map->n_buckets; i++) {
        RDFTermHashEntry *entry = map->buckets[i];
        while (entry != NULL) {
            RDFTermHashEntry *next = entry->next;
            rdfterm_free_fields(&entry->key);
            free(entry);
            entry = next;
        }
    }
    free(map->buckets);
}

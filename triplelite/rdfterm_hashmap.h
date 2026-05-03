#ifndef RDFTERM_HASHMAP_H
#define RDFTERM_HASHMAP_H

#include <stddef.h>
#include "dynarray.h"

typedef struct RDFTermHashEntry {
    RDFTerm key;
    size_t value;
    struct RDFTermHashEntry *next;
} RDFTermHashEntry;

typedef struct {
    RDFTermHashEntry **buckets;
    size_t n_buckets;
    size_t len;
} RDFTermHashMap;

int rdfterm_hashmap_init(RDFTermHashMap *map, size_t n_buckets);
int rdfterm_hashmap_get(RDFTermHashMap *map, const RDFTerm *key, size_t *out);
int rdfterm_hashmap_put(RDFTermHashMap *map, const RDFTerm *key, size_t value);
void rdfterm_hashmap_free(RDFTermHashMap *map);

#endif

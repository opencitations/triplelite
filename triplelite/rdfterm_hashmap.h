/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#ifndef RDFTERM_HASHMAP_H
#define RDFTERM_HASHMAP_H

#include "oa_hash.h"
#include "dynarray.h"

typedef struct {
    RDFTerm key;
    size_t value;
} RDFTermSlot;

typedef OATable RDFTermHashMap;

int rdfterm_hashmap_init(RDFTermHashMap *map, size_t n_slots);
int rdfterm_hashmap_get(RDFTermHashMap *map, const RDFTerm *key, size_t *out);
int rdfterm_hashmap_put(RDFTermHashMap *map, const RDFTerm *key, size_t value);
void rdfterm_hashmap_free(RDFTermHashMap *map);

#endif

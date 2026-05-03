/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#include "rdfterm_hashmap.h"
#include <stdint.h>

#define RDFTERM_EMPTY SIZE_MAX

static int is_empty(const void *slot)
{
    return ((const RDFTermSlot *)slot)->key.type_id == RDFTERM_EMPTY;
}

static void set_empty(void *slot)
{
    ((RDFTermSlot *)slot)->key.type_id = RDFTERM_EMPTY;
}

static size_t hash(const void *slot)
{
    const RDFTerm *k = &((const RDFTermSlot *)slot)->key;
    size_t h = k->type_id;
    h = h * 31 + k->value_id;
    h = h * 31 + k->datatype_id;
    h = h * 31 + k->lang_id;
    return h;
}

static int equal(const void *a, const void *b)
{
    const RDFTerm *ka = &((const RDFTermSlot *)a)->key;
    const RDFTerm *kb = &((const RDFTermSlot *)b)->key;
    return ka->type_id == kb->type_id &&
           ka->value_id == kb->value_id &&
           ka->datatype_id == kb->datatype_id &&
           ka->lang_id == kb->lang_id;
}

static const OAOps rdfterm_ops = {
    sizeof(RDFTermSlot), is_empty, set_empty, hash, equal, NULL
};

int rdfterm_hashmap_init(RDFTermHashMap *map, size_t n_slots)
{
    return oa_init(map, n_slots, &rdfterm_ops);
}

int rdfterm_hashmap_get(RDFTermHashMap *map, const RDFTerm *key, size_t *out)
{
    RDFTermSlot search = {*key, 0};
    RDFTermSlot *found = oa_find(map, &search, &rdfterm_ops);
    if (found == NULL)
        return 0;
    *out = found->value;
    return 1;
}

int rdfterm_hashmap_put(RDFTermHashMap *map, const RDFTerm *key, size_t value)
{
    if (map->len * 4 >= map->n_slots * 3) {
        if (oa_grow(map, &rdfterm_ops) < 0)
            return -1;
    }
    RDFTermSlot search = {*key, 0};
    size_t idx = oa_probe(map, &search, &rdfterm_ops);
    RDFTermSlot *slot = (RDFTermSlot *)map->slots + idx;
    if (slot->key.type_id != RDFTERM_EMPTY) {
        slot->value = value;
        return 0;
    }
    slot->key = *key;
    slot->value = value;
    map->len++;
    return 0;
}

void rdfterm_hashmap_free(RDFTermHashMap *map)
{
    oa_free(map, &rdfterm_ops);
}

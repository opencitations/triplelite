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
    const RDFTerm *key = &((const RDFTermSlot *)slot)->key;
    size_t hash_value = key->type_id;
    hash_value = hash_value * 31 + key->value_id;
    hash_value = hash_value * 31 + key->datatype_id;
    hash_value = hash_value * 31 + key->lang_id;
    return hash_value;
}

static int equal(const void *slot_a, const void *slot_b)
{
    const RDFTerm *key_a = &((const RDFTermSlot *)slot_a)->key;
    const RDFTerm *key_b = &((const RDFTermSlot *)slot_b)->key;
    return key_a->type_id == key_b->type_id &&
           key_a->value_id == key_b->value_id &&
           key_a->datatype_id == key_b->datatype_id &&
           key_a->lang_id == key_b->lang_id;
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
    size_t slot_index = oa_probe(map, &search, &rdfterm_ops);
    RDFTermSlot *slot = (RDFTermSlot *)map->slots + slot_index;
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

/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#include "hashmap.h"
#include <string.h>

static size_t hash_string(const char *str)
{
    size_t hash_value = 5381;
    int ch;
    while ((ch = *str++))
        hash_value = ((hash_value << 5) + hash_value) + ch;
    return hash_value;
}

static int is_empty(const void *slot)
{
    return ((const HashSlot *)slot)->key == NULL;
}

static void set_empty(void *slot)
{
    ((HashSlot *)slot)->key = NULL;
}

static size_t hash(const void *slot)
{
    return hash_string(((const HashSlot *)slot)->key);
}

static int equal(const void *slot_a, const void *slot_b)
{
    return strcmp(((const HashSlot *)slot_a)->key, ((const HashSlot *)slot_b)->key) == 0;
}

static const OAOps hashmap_ops = {
    sizeof(HashSlot), is_empty, set_empty, hash, equal, NULL
};

int hashmap_init(HashMap *map, size_t n_slots)
{
    return oa_init(map, n_slots, &hashmap_ops);
}

int hashmap_get(HashMap *map, const char *key, size_t *out)
{
    HashSlot search = {key, 0};
    HashSlot *found = oa_find(map, &search, &hashmap_ops);
    if (found == NULL)
        return 0;
    *out = found->value;
    return 1;
}

int hashmap_put(HashMap *map, const char *key, size_t value)
{
    if (map->len * 4 >= map->n_slots * 3) {
        if (oa_grow(map, &hashmap_ops) < 0)
            return -1;
    }
    HashSlot search = {key, 0};
    size_t slot_index = oa_probe(map, &search, &hashmap_ops);
    HashSlot *slot = (HashSlot *)map->slots + slot_index;
    if (slot->key != NULL) {
        slot->value = value;
        return 0;
    }
    slot->key = key;
    slot->value = value;
    map->len++;
    return 0;
}

void hashmap_free(HashMap *map)
{
    oa_free(map, &hashmap_ops);
}

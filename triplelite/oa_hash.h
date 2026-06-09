/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#ifndef OA_HASH_H
#define OA_HASH_H

#include <stddef.h>
#include <stdint.h>

static inline size_t oa_mix64(size_t key)
{
    uint64_t mixed = (uint64_t)key;
    mixed = (mixed ^ (mixed >> 30)) * 0xbf58476d1ce4e5b9ULL;
    mixed = (mixed ^ (mixed >> 27)) * 0x94d049bb133111ebULL;
    mixed = mixed ^ (mixed >> 31);
    return (size_t)mixed;
}

typedef struct {
    size_t slot_size;
    int (*is_empty)(const void *slot);
    void (*set_empty)(void *slot);
    size_t (*hash)(const void *slot);
    int (*equal)(const void *slot_a, const void *slot_b);
    void (*cleanup)(void *slot);
} OAOps;

typedef struct {
    void *slots;
    size_t n_slots;
    size_t len;
} OATable;

int oa_init(OATable *table, size_t n_slots, const OAOps *ops);
void *oa_find(OATable *table, const void *key, const OAOps *ops);
size_t oa_probe(OATable *table, const void *key, const OAOps *ops);
int oa_grow(OATable *table, const OAOps *ops);
void oa_remove_at(OATable *table, size_t slot_index, const OAOps *ops);
void oa_free(OATable *table, const OAOps *ops);

#endif

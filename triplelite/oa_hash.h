/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#ifndef OA_HASH_H
#define OA_HASH_H

#include <stddef.h>

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

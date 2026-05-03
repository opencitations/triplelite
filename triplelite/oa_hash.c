/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#include "oa_hash.h"
#include <stdlib.h>
#include <string.h>

static inline void *slot_at(OATable *t, size_t i, const OAOps *ops)
{
    return (char *)t->slots + i * ops->slot_size;
}

int oa_init(OATable *t, size_t n_slots, const OAOps *ops)
{
    t->slots = malloc(n_slots * ops->slot_size);
    if (t->slots == NULL)
        return -1;
    for (size_t i = 0; i < n_slots; i++)
        ops->set_empty(slot_at(t, i, ops));
    t->n_slots = n_slots;
    t->len = 0;
    return 0;
}

void *oa_find(OATable *t, const void *key, const OAOps *ops)
{
    size_t idx = ops->hash(key) % t->n_slots;
    for (;;) {
        void *slot = slot_at(t, idx, ops);
        if (ops->is_empty(slot))
            return NULL;
        if (ops->equal(slot, key))
            return slot;
        idx = (idx + 1) % t->n_slots;
    }
}

size_t oa_probe(OATable *t, const void *key, const OAOps *ops)
{
    size_t idx = ops->hash(key) % t->n_slots;
    for (;;) {
        void *slot = slot_at(t, idx, ops);
        if (ops->is_empty(slot) || ops->equal(slot, key))
            return idx;
        idx = (idx + 1) % t->n_slots;
    }
}

int oa_grow(OATable *t, const OAOps *ops)
{
    size_t old_n = t->n_slots;
    void *old_slots = t->slots;
    size_t new_n = old_n * 2;
    t->slots = malloc(new_n * ops->slot_size);
    if (t->slots == NULL) {
        t->slots = old_slots;
        return -1;
    }
    for (size_t i = 0; i < new_n; i++)
        ops->set_empty(slot_at(t, i, ops));
    t->n_slots = new_n;
    t->len = 0;
    for (size_t i = 0; i < old_n; i++) {
        void *old_slot = (char *)old_slots + i * ops->slot_size;
        if (!ops->is_empty(old_slot)) {
            size_t idx = oa_probe(t, old_slot, ops);
            memcpy(slot_at(t, idx, ops), old_slot, ops->slot_size);
            t->len++;
        }
    }
    free(old_slots);
    return 0;
}

void oa_remove_at(OATable *t, size_t idx, const OAOps *ops)
{
    if (ops->cleanup)
        ops->cleanup(slot_at(t, idx, ops));
    ops->set_empty(slot_at(t, idx, ops));
    t->len--;
    size_t next = (idx + 1) % t->n_slots;
    while (!ops->is_empty(slot_at(t, next, ops))) {
        void *next_slot = slot_at(t, next, ops);
        size_t natural = ops->hash(next_slot) % t->n_slots;
        if ((next > idx && (natural <= idx || natural > next)) ||
            (next < idx && (natural <= idx && natural > next))) {
            memcpy(slot_at(t, idx, ops), next_slot, ops->slot_size);
            ops->set_empty(next_slot);
            idx = next;
        }
        next = (next + 1) % t->n_slots;
    }
}

void oa_free(OATable *t, const OAOps *ops)
{
    if (t->slots) {
        if (ops->cleanup) {
            for (size_t i = 0; i < t->n_slots; i++) {
                void *slot = slot_at(t, i, ops);
                if (!ops->is_empty(slot))
                    ops->cleanup(slot);
            }
        }
        free(t->slots);
    }
    t->slots = NULL;
    t->n_slots = 0;
    t->len = 0;
}

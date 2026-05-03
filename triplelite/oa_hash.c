/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#include "oa_hash.h"
#include <stdlib.h>
#include <string.h>

static inline void *slot_at(OATable *table, size_t i, const OAOps *ops)
{
    return (char *)table->slots + i * ops->slot_size;
}

int oa_init(OATable *table, size_t n_slots, const OAOps *ops)
{
    table->slots = malloc(n_slots * ops->slot_size);
    if (table->slots == NULL)
        return -1;
    for (size_t i = 0; i < n_slots; i++)
        ops->set_empty(slot_at(table, i, ops));
    table->n_slots = n_slots;
    table->len = 0;
    return 0;
}

void *oa_find(OATable *table, const void *key, const OAOps *ops)
{
    size_t slot_index = ops->hash(key) % table->n_slots;
    for (;;) {
        void *slot = slot_at(table, slot_index, ops);
        if (ops->is_empty(slot))
            return NULL;
        if (ops->equal(slot, key))
            return slot;
        slot_index = (slot_index + 1) % table->n_slots;
    }
}

size_t oa_probe(OATable *table, const void *key, const OAOps *ops)
{
    size_t slot_index = ops->hash(key) % table->n_slots;
    for (;;) {
        void *slot = slot_at(table, slot_index, ops);
        if (ops->is_empty(slot) || ops->equal(slot, key))
            return slot_index;
        slot_index = (slot_index + 1) % table->n_slots;
    }
}

int oa_grow(OATable *table, const OAOps *ops)
{
    size_t old_n_slots = table->n_slots;
    void *old_slots = table->slots;
    size_t new_n_slots = old_n_slots * 2;
    table->slots = malloc(new_n_slots * ops->slot_size);
    if (table->slots == NULL) {
        table->slots = old_slots;
        return -1;
    }
    for (size_t i = 0; i < new_n_slots; i++)
        ops->set_empty(slot_at(table, i, ops));
    table->n_slots = new_n_slots;
    table->len = 0;
    for (size_t i = 0; i < old_n_slots; i++) {
        void *old_slot = (char *)old_slots + i * ops->slot_size;
        if (!ops->is_empty(old_slot)) {
            size_t slot_index = oa_probe(table, old_slot, ops);
            memcpy(slot_at(table, slot_index, ops), old_slot, ops->slot_size);
            table->len++;
        }
    }
    free(old_slots);
    return 0;
}

void oa_remove_at(OATable *table, size_t slot_index, const OAOps *ops)
{
    if (ops->cleanup)
        ops->cleanup(slot_at(table, slot_index, ops));
    ops->set_empty(slot_at(table, slot_index, ops));
    table->len--;
    size_t next_index = (slot_index + 1) % table->n_slots;
    while (!ops->is_empty(slot_at(table, next_index, ops))) {
        void *next_slot = slot_at(table, next_index, ops);
        size_t natural = ops->hash(next_slot) % table->n_slots;
        if ((next_index > slot_index && (natural <= slot_index || natural > next_index)) ||
            (next_index < slot_index && (natural <= slot_index && natural > next_index))) {
            memcpy(slot_at(table, slot_index, ops), next_slot, ops->slot_size);
            ops->set_empty(next_slot);
            slot_index = next_index;
        }
        next_index = (next_index + 1) % table->n_slots;
    }
}

void oa_free(OATable *table, const OAOps *ops)
{
    if (table->slots) {
        if (ops->cleanup) {
            for (size_t i = 0; i < table->n_slots; i++) {
                void *slot = slot_at(table, i, ops);
                if (!ops->is_empty(slot))
                    ops->cleanup(slot);
            }
        }
        free(table->slots);
    }
    table->slots = NULL;
    table->n_slots = 0;
    table->len = 0;
}

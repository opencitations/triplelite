/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#include "intset.h"

static int is_empty(const void *slot)
{
    return ((const IntSetSlot *)slot)->value == INTSET_EMPTY;
}

static void set_empty(void *slot)
{
    ((IntSetSlot *)slot)->value = INTSET_EMPTY;
}

static size_t hash(const void *slot)
{
    return ((const IntSetSlot *)slot)->value;
}

static int equal(const void *slot_a, const void *slot_b)
{
    return ((const IntSetSlot *)slot_a)->value == ((const IntSetSlot *)slot_b)->value;
}

static const OAOps intset_ops = {
    sizeof(IntSetSlot), is_empty, set_empty, hash, equal, NULL
};

int intset_init(IntSet *set, size_t n_slots)
{
    return oa_init(set, n_slots, &intset_ops);
}

int intset_add(IntSet *set, size_t value)
{
    if (set->len * 4 >= set->n_slots * 3) {
        if (oa_grow(set, &intset_ops) < 0)
            return -1;
    }
    IntSetSlot key = {value};
    size_t slot_index = oa_probe(set, &key, &intset_ops);
    IntSetSlot *slot = (IntSetSlot *)set->slots + slot_index;
    if (slot->value != INTSET_EMPTY)
        return 0;
    slot->value = value;
    set->len++;
    return 1;
}

int intset_contains(IntSet *set, size_t value)
{
    IntSetSlot key = {value};
    return oa_find(set, &key, &intset_ops) != NULL;
}

void intset_remove(IntSet *set, size_t value)
{
    IntSetSlot key = {value};
    size_t slot_index = oa_probe(set, &key, &intset_ops);
    IntSetSlot *slot = (IntSetSlot *)set->slots + slot_index;
    if (slot->value == INTSET_EMPTY)
        return;
    oa_remove_at(set, slot_index, &intset_ops);
}

void intset_free(IntSet *set)
{
    oa_free(set, &intset_ops);
}

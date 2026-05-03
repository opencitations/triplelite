/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#ifndef INTSET_H
#define INTSET_H

#include "oa_hash.h"
#include <stdint.h>

#define INTSET_EMPTY SIZE_MAX

typedef struct {
    size_t value;
} IntSetSlot;

typedef OATable IntSet;

int intset_init(IntSet *set, size_t n_slots);
int intset_add(IntSet *set, size_t value);
int intset_contains(IntSet *set, size_t value);
void intset_remove(IntSet *set, size_t value);
void intset_free(IntSet *set);

#endif

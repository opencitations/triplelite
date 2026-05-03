/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#ifndef HASHMAP_H
#define HASHMAP_H

#include "oa_hash.h"

typedef struct {
    const char *key;
    size_t value;
} HashSlot;

typedef OATable HashMap;

int hashmap_init(HashMap *map, size_t n_slots);
int hashmap_get(HashMap *map, const char *key, size_t *out);
int hashmap_put(HashMap *map, const char *key, size_t value);
void hashmap_free(HashMap *map);

#endif

#ifndef INTSET_H
#define INTSET_H

#include <stddef.h>

typedef struct {
    size_t *slots;
    char *occupied;
    size_t n_slots;
    size_t len;
} IntSet;

int intset_init(IntSet *set, size_t n_slots);
int intset_add(IntSet *set, size_t value);
int intset_contains(IntSet *set, size_t value);
void intset_remove(IntSet *set, size_t value);
void intset_free(IntSet *set);

#endif

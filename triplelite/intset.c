#include "intset.h"
#include <stdlib.h>
#include <string.h>

int intset_init(IntSet *set, size_t n_slots)
{
    set->slots = malloc(n_slots * sizeof(size_t));
    if (set->slots == NULL) {
        return -1;
    }
    set->occupied = calloc(n_slots, sizeof(char));
    if (set->occupied == NULL) {
        free(set->slots);
        return -1;
    }
    set->n_slots = n_slots;
    set->len = 0;
    return 0;
}

static size_t intset_probe(IntSet *set, size_t value)
{
    size_t idx = value % set->n_slots;
    while (set->occupied[idx] && set->slots[idx] != value) {
        idx = (idx + 1) % set->n_slots;
    }
    return idx;
}

static int intset_grow(IntSet *set)
{
    size_t old_capacity = set->n_slots;
    size_t *old_slots = set->slots;
    char *old_occupied = set->occupied;

    size_t new_capacity = old_capacity * 2;
    set->slots = malloc(new_capacity * sizeof(size_t));
    if (set->slots == NULL) {
        set->slots = old_slots;
        set->occupied = old_occupied;
        return -1;
    }
    set->occupied = calloc(new_capacity, sizeof(char));
    if (set->occupied == NULL) {
        free(set->slots);
        set->slots = old_slots;
        set->occupied = old_occupied;
        return -1;
    }
    set->n_slots = new_capacity;
    set->len = 0;

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_occupied[i]) {
            size_t idx = intset_probe(set, old_slots[i]);
            set->slots[idx] = old_slots[i];
            set->occupied[idx] = 1;
            set->len++;
        }
    }
    free(old_slots);
    free(old_occupied);
    return 0;
}

int intset_add(IntSet *set, size_t value)
{
    if (set->len * 4 >= set->n_slots * 3) {
        if (intset_grow(set) < 0) {
            return -1;
        }
    }
    size_t idx = intset_probe(set, value);
    if (set->occupied[idx]) {
        return 0;
    }
    set->slots[idx] = value;
    set->occupied[idx] = 1;
    set->len++;
    return 1;
}

int intset_contains(IntSet *set, size_t value)
{
    size_t idx = intset_probe(set, value);
    return set->occupied[idx];
}

void intset_remove(IntSet *set, size_t value)
{
    size_t idx = value % set->n_slots;
    while (set->occupied[idx]) {
        if (set->slots[idx] == value) {
            set->occupied[idx] = 0;
            set->len--;
            size_t next = (idx + 1) % set->n_slots;
            while (set->occupied[next]) {
                size_t natural = set->slots[next] % set->n_slots;
                if ((next > idx && (natural <= idx || natural > next)) ||
                    (next < idx && (natural <= idx && natural > next))) {
                    set->slots[idx] = set->slots[next];
                    set->occupied[idx] = 1;
                    set->occupied[next] = 0;
                    idx = next;
                }
                next = (next + 1) % set->n_slots;
            }
            return;
        }
        idx = (idx + 1) % set->n_slots;
    }
}

void intset_free(IntSet *set)
{
    free(set->slots);
    free(set->occupied);
    set->slots = NULL;
    set->occupied = NULL;
    set->n_slots = 0;
    set->len = 0;
}

/*
 * SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 *
 * SPDX-License-Identifier: ISC
 */

#include "dynarray.h"
#include <stdlib.h>
#include <string.h>

void string_array_init(StringArray *arr)
{
    arr->items = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

int string_array_append(StringArray *arr, const char *str)
{
    if (arr->len == arr->capacity) {
        size_t new_capacity = arr->capacity == 0 ? 8 : arr->capacity * 2;
        const char **new_items = realloc(arr->items, new_capacity * sizeof(const char *));
        if (new_items == NULL) {
            return -1;
        }
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    arr->items[arr->len] = strdup(str);
    if (arr->items[arr->len] == NULL) {
        return -1;
    }
    arr->len++;
    return 0;
}

void string_array_free(StringArray *arr)
{
    for (size_t i = 0; i < arr->len; i++) {
        free((char *)arr->items[i]);
    }
    free(arr->items);
}

void rdfterm_array_init(RDFTermArray *arr)
{
    arr->items = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

int rdfterm_array_append(RDFTermArray *arr, const RDFTerm *term)
{
    if (arr->len == arr->capacity) {
        size_t new_capacity = arr->capacity == 0 ? 8 : arr->capacity * 2;
        RDFTerm *new_items = realloc(arr->items, new_capacity * sizeof(RDFTerm));
        if (new_items == NULL) {
            return -1;
        }
        arr->items = new_items;
        arr->capacity = new_capacity;
    }
    arr->items[arr->len] = *term;
    arr->len++;
    return 0;
}

void rdfterm_array_free(RDFTermArray *arr)
{
    free(arr->items);
}

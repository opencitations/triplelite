/*
 * SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 *
 * SPDX-License-Identifier: ISC
 */

#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <stddef.h>

typedef struct {
    size_t type_id;
    size_t value_id;
    size_t datatype_id;
    size_t lang_id;
} RDFTerm;

typedef struct {
    const char **items;
    size_t len;
    size_t capacity;
} StringArray;

typedef struct {
    RDFTerm *items;
    size_t len;
    size_t capacity;
} RDFTermArray;

void string_array_init(StringArray *arr);
int string_array_append(StringArray *arr, const char *str);
void string_array_free(StringArray *arr);
void rdfterm_array_init(RDFTermArray *arr);
int rdfterm_array_append(RDFTermArray *arr, const RDFTerm *term);
void rdfterm_array_free(RDFTermArray *arr);

#endif

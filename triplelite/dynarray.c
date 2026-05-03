#include "dynarray.h"
#include <stdlib.h>
#include <string.h>

void string_array_init(StringArray *arr)
{
    arr->items = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

int string_array_append(StringArray *arr, const char *s){
    if (arr->len == arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? 8 : arr->capacity * 2;
        const char **new_items = realloc(arr->items, new_cap * sizeof(const char *));
        if (new_items == NULL) {
            return -1;
        }
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    arr->items[arr->len] = strdup(s);
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

void rdfterm_free_fields(RDFTerm *term)
{
    free((char *)term->type);
    free((char *)term->value);
    free((char *)term->datatype);
    free((char *)term->lang);
}

int rdfterm_copy(RDFTerm *dst, const RDFTerm *src)
{
    dst->type = strdup(src->type);
    dst->value = strdup(src->value);
    dst->datatype = strdup(src->datatype);
    dst->lang = strdup(src->lang);
    if (dst->type == NULL || dst->value == NULL ||
        dst->datatype == NULL || dst->lang == NULL) {
        rdfterm_free_fields(dst);
        return -1;
    }
    return 0;
}

void rdfterm_array_init(RDFTermArray *arr)
{
    arr->items = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

int rdfterm_array_append(RDFTermArray *arr, const RDFTerm *term){
    if (arr->len == arr->capacity) {
        size_t new_cap = arr->capacity == 0 ? 8 : arr->capacity * 2;
        RDFTerm *new_items = realloc(arr->items, new_cap * sizeof(RDFTerm));
        if (new_items == NULL) {
            return -1;
        }
        arr->items = new_items;
        arr->capacity = new_cap;
    }
    if (rdfterm_copy(&arr->items[arr->len], term) < 0) {
        return -1;
    }
    arr->len++;
    return 0;
}

void rdfterm_array_free(RDFTermArray *arr)
{
    for (size_t i = 0; i < arr->len; i++) {
        rdfterm_free_fields(&arr->items[i]);
    }
    free(arr->items);
}

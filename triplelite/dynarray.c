#include "dynarray.h"
#include <stdlib.h>

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
    arr->items[arr->len] = s;
    arr->len++;
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
    arr->items[arr->len] = *term;
    arr->len++;
    return 0;
}

#include "spo_index.h"
#include <stdlib.h>

int spo_init(SPOIndex *idx, size_t n_buckets)
{
    idx->buckets = calloc(n_buckets, sizeof(SubjEntry *));
    if (idx->buckets == NULL) {
        return -1;
    }
    idx->n_buckets = n_buckets;
    idx->len = 0;
    return 0;
}

static PredEntry *predmap_find(PredMap *pm, size_t pred_id)
{
    size_t slot = pred_id % pm->n_buckets;
    PredEntry *e = pm->buckets[slot];
    while (e != NULL) {
        if (e->pred_id == pred_id) {
            return e;
        }
        e = e->next;
    }
    return NULL;
}

static PredEntry *predmap_get_or_create(PredMap *pm, size_t pred_id)
{
    size_t slot = pred_id % pm->n_buckets;
    PredEntry *e = pm->buckets[slot];
    while (e != NULL) {
        if (e->pred_id == pred_id) {
            return e;
        }
        e = e->next;
    }
    e = malloc(sizeof(PredEntry));
    if (e == NULL) {
        return NULL;
    }
    if (intset_init(&e->objects, 4) < 0) {
        free(e);
        return NULL;
    }
    e->pred_id = pred_id;
    e->next = pm->buckets[slot];
    pm->buckets[slot] = e;
    pm->len++;
    return e;
}

static int predmap_init(PredMap *pm, size_t n_buckets)
{
    pm->buckets = calloc(n_buckets, sizeof(PredEntry *));
    if (pm->buckets == NULL) {
        return -1;
    }
    pm->n_buckets = n_buckets;
    pm->len = 0;
    return 0;
}

static SubjEntry *spo_find(SPOIndex *idx, size_t subj_id)
{
    size_t slot = subj_id % idx->n_buckets;
    SubjEntry *e = idx->buckets[slot];
    while (e != NULL) {
        if (e->subj_id == subj_id) {
            return e;
        }
        e = e->next;
    }
    return NULL;
}

static SubjEntry *spo_get_or_create(SPOIndex *idx, size_t subj_id)
{
    size_t slot = subj_id % idx->n_buckets;
    SubjEntry *e = idx->buckets[slot];
    while (e != NULL) {
        if (e->subj_id == subj_id) {
            return e;
        }
        e = e->next;
    }
    e = malloc(sizeof(SubjEntry));
    if (e == NULL) {
        return NULL;
    }
    if (predmap_init(&e->predicates, 4) < 0) {
        free(e);
        return NULL;
    }
    e->subj_id = subj_id;
    e->next = idx->buckets[slot];
    idx->buckets[slot] = e;
    idx->len++;
    return e;
}

int spo_add(SPOIndex *idx, size_t sid, size_t pid, size_t oid)
{
    SubjEntry *subj = spo_get_or_create(idx, sid);
    if (subj == NULL) {
        return -1;
    }
    PredEntry *pred = predmap_get_or_create(&subj->predicates, pid);
    if (pred == NULL) {
        return -1;
    }
    return intset_add(&pred->objects, oid);
}

int spo_contains(SPOIndex *idx, size_t sid, size_t pid, size_t oid)
{
    SubjEntry *subj = spo_find(idx, sid);
    if (subj == NULL) {
        return 0;
    }
    PredEntry *pred = predmap_find(&subj->predicates, pid);
    if (pred == NULL) {
        return 0;
    }
    return intset_contains(&pred->objects, oid);
}

int spo_remove(SPOIndex *idx, size_t sid, size_t pid, size_t oid)
{
    SubjEntry *subj = spo_find(idx, sid);
    if (subj == NULL) {
        return 0;
    }
    PredEntry *pred = predmap_find(&subj->predicates, pid);
    if (pred == NULL) {
        return 0;
    }
    if (!intset_contains(&pred->objects, oid)) {
        return 0;
    }
    intset_remove(&pred->objects, oid);
    return 1;
}

PredMap *spo_get_preds(SPOIndex *idx, size_t sid)
{
    SubjEntry *subj = spo_find(idx, sid);
    if (subj == NULL) {
        return NULL;
    }
    return &subj->predicates;
}

IntSet *spo_get_objects(SPOIndex *idx, size_t sid, size_t pid)
{
    SubjEntry *subj = spo_find(idx, sid);
    if (subj == NULL) {
        return NULL;
    }
    PredEntry *pred = predmap_find(&subj->predicates, pid);
    if (pred == NULL) {
        return NULL;
    }
    return &pred->objects;
}

static void predmap_free(PredMap *pm)
{
    for (size_t i = 0; i < pm->n_buckets; i++) {
        PredEntry *e = pm->buckets[i];
        while (e != NULL) {
            PredEntry *next = e->next;
            intset_free(&e->objects);
            free(e);
            e = next;
        }
    }
    free(pm->buckets);
}

void spo_free(SPOIndex *idx)
{
    for (size_t i = 0; i < idx->n_buckets; i++) {
        SubjEntry *e = idx->buckets[i];
        while (e != NULL) {
            SubjEntry *next = e->next;
            predmap_free(&e->predicates);
            free(e);
            e = next;
        }
    }
    free(idx->buckets);
    idx->buckets = NULL;
    idx->n_buckets = 0;
    idx->len = 0;
}

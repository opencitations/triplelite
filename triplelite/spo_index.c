#include "spo_index.h"
#include <stdlib.h>

static void predmap_free(PredMap *pred_map);

int spo_init(SPOIndex *index, size_t n_buckets)
{
    index->buckets = calloc(n_buckets, sizeof(SubjEntry *));
    if (index->buckets == NULL) {
        return -1;
    }
    index->n_buckets = n_buckets;
    index->len = 0;
    return 0;
}

static PredEntry *predmap_find(PredMap *pred_map, size_t predicate_id)
{
    size_t slot = predicate_id % pred_map->n_buckets;
    PredEntry *entry = pred_map->buckets[slot];
    while (entry != NULL) {
        if (entry->pred_id == predicate_id) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static PredEntry *predmap_get_or_create(PredMap *pred_map, size_t predicate_id)
{
    size_t slot = predicate_id % pred_map->n_buckets;
    PredEntry *entry = pred_map->buckets[slot];
    while (entry != NULL) {
        if (entry->pred_id == predicate_id) {
            return entry;
        }
        entry = entry->next;
    }
    entry = malloc(sizeof(PredEntry));
    if (entry == NULL) {
        return NULL;
    }
    if (intset_init(&entry->objects, 4) < 0) {
        free(entry);
        return NULL;
    }
    entry->pred_id = predicate_id;
    entry->next = pred_map->buckets[slot];
    pred_map->buckets[slot] = entry;
    pred_map->len++;
    return entry;
}

static int predmap_init(PredMap *pred_map, size_t n_buckets)
{
    pred_map->buckets = calloc(n_buckets, sizeof(PredEntry *));
    if (pred_map->buckets == NULL) {
        return -1;
    }
    pred_map->n_buckets = n_buckets;
    pred_map->len = 0;
    return 0;
}

static SubjEntry *spo_find(SPOIndex *index, size_t subject_id)
{
    size_t slot = subject_id % index->n_buckets;
    SubjEntry *entry = index->buckets[slot];
    while (entry != NULL) {
        if (entry->subj_id == subject_id) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static SubjEntry *spo_get_or_create(SPOIndex *index, size_t subject_id)
{
    size_t slot = subject_id % index->n_buckets;
    SubjEntry *entry = index->buckets[slot];
    while (entry != NULL) {
        if (entry->subj_id == subject_id) {
            return entry;
        }
        entry = entry->next;
    }
    entry = malloc(sizeof(SubjEntry));
    if (entry == NULL) {
        return NULL;
    }
    if (predmap_init(&entry->predicates, 4) < 0) {
        free(entry);
        return NULL;
    }
    entry->subj_id = subject_id;
    entry->next = index->buckets[slot];
    index->buckets[slot] = entry;
    index->len++;
    return entry;
}

int spo_add(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id)
{
    SubjEntry *subject = spo_get_or_create(index, subject_id);
    if (subject == NULL) {
        return -1;
    }
    PredEntry *predicate = predmap_get_or_create(&subject->predicates, predicate_id);
    if (predicate == NULL) {
        return -1;
    }
    return intset_add(&predicate->objects, object_id);
}

int spo_contains(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id)
{
    SubjEntry *subject = spo_find(index, subject_id);
    if (subject == NULL) {
        return 0;
    }
    PredEntry *predicate = predmap_find(&subject->predicates, predicate_id);
    if (predicate == NULL) {
        return 0;
    }
    return intset_contains(&predicate->objects, object_id);
}

static void predmap_remove_entry(PredMap *pred_map, size_t predicate_id)
{
    size_t slot = predicate_id % pred_map->n_buckets;
    PredEntry *prev = NULL;
    PredEntry *entry = pred_map->buckets[slot];
    while (entry != NULL) {
        if (entry->pred_id == predicate_id) {
            if (prev == NULL) {
                pred_map->buckets[slot] = entry->next;
            } else {
                prev->next = entry->next;
            }
            intset_free(&entry->objects);
            free(entry);
            pred_map->len--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

static void spo_remove_entry(SPOIndex *index, size_t subject_id)
{
    size_t slot = subject_id % index->n_buckets;
    SubjEntry *prev = NULL;
    SubjEntry *entry = index->buckets[slot];
    while (entry != NULL) {
        if (entry->subj_id == subject_id) {
            if (prev == NULL) {
                index->buckets[slot] = entry->next;
            } else {
                prev->next = entry->next;
            }
            predmap_free(&entry->predicates);
            free(entry);
            index->len--;
            return;
        }
        prev = entry;
        entry = entry->next;
    }
}

int spo_remove(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id)
{
    SubjEntry *subject = spo_find(index, subject_id);
    if (subject == NULL) {
        return 0;
    }
    PredEntry *predicate = predmap_find(&subject->predicates, predicate_id);
    if (predicate == NULL) {
        return 0;
    }
    if (!intset_contains(&predicate->objects, object_id)) {
        return 0;
    }
    intset_remove(&predicate->objects, object_id);
    if (predicate->objects.len == 0) {
        predmap_remove_entry(&subject->predicates, predicate_id);
        if (subject->predicates.len == 0) {
            spo_remove_entry(index, subject_id);
        }
    }
    return 1;
}

PredMap *spo_get_preds(SPOIndex *index, size_t subject_id)
{
    SubjEntry *subject = spo_find(index, subject_id);
    if (subject == NULL) {
        return NULL;
    }
    return &subject->predicates;
}

IntSet *spo_get_objects(SPOIndex *index, size_t subject_id, size_t predicate_id)
{
    SubjEntry *subject = spo_find(index, subject_id);
    if (subject == NULL) {
        return NULL;
    }
    PredEntry *predicate = predmap_find(&subject->predicates, predicate_id);
    if (predicate == NULL) {
        return NULL;
    }
    return &predicate->objects;
}

static void predmap_free(PredMap *pred_map)
{
    for (size_t i = 0; i < pred_map->n_buckets; i++) {
        PredEntry *entry = pred_map->buckets[i];
        while (entry != NULL) {
            PredEntry *next = entry->next;
            intset_free(&entry->objects);
            free(entry);
            entry = next;
        }
    }
    free(pred_map->buckets);
}

void spo_free(SPOIndex *index)
{
    for (size_t i = 0; i < index->n_buckets; i++) {
        SubjEntry *entry = index->buckets[i];
        while (entry != NULL) {
            SubjEntry *next = entry->next;
            predmap_free(&entry->predicates);
            free(entry);
            entry = next;
        }
    }
    free(index->buckets);
    index->buckets = NULL;
    index->n_buckets = 0;
    index->len = 0;
}

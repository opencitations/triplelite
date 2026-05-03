#ifndef SPO_INDEX_H
#define SPO_INDEX_H

#include <stddef.h>
#include "intset.h"

typedef struct PredEntry {
    size_t pred_id;
    IntSet objects;
    struct PredEntry *next;
} PredEntry;

typedef struct {
    PredEntry **buckets;
    size_t n_buckets;
    size_t len;
} PredMap;

typedef struct SubjEntry {
    size_t subj_id;
    PredMap predicates;
    struct SubjEntry *next;
} SubjEntry;

typedef struct {
    SubjEntry **buckets;
    size_t n_buckets;
    size_t len;
} SPOIndex;

int spo_init(SPOIndex *index, size_t n_buckets);
int spo_add(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id);
int spo_contains(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id);
int spo_remove(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id);
PredMap *spo_get_preds(SPOIndex *index, size_t subject_id);
IntSet *spo_get_objects(SPOIndex *index, size_t subject_id, size_t predicate_id);
void spo_free(SPOIndex *index);

#endif

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

int spo_init(SPOIndex *idx, size_t n_buckets);
int spo_add(SPOIndex *idx, size_t sid, size_t pid, size_t oid);
int spo_contains(SPOIndex *idx, size_t sid, size_t pid, size_t oid);
int spo_remove(SPOIndex *idx, size_t sid, size_t pid, size_t oid);
PredMap *spo_get_preds(SPOIndex *idx, size_t sid);
IntSet *spo_get_objects(SPOIndex *idx, size_t sid, size_t pid);
void spo_free(SPOIndex *idx);

#endif

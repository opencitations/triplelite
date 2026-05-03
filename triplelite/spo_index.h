/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#ifndef SPO_INDEX_H
#define SPO_INDEX_H

#include "oa_hash.h"
#include "intset.h"

#define SPO_EMPTY SIZE_MAX

typedef struct {
    size_t pred_id;
    IntSet objects;
} PredSlot;

typedef OATable PredMap;

typedef struct {
    size_t subj_id;
    PredMap predicates;
} SubjSlot;

typedef OATable SPOIndex;

int spo_init(SPOIndex *index, size_t n_slots);
SubjSlot *spo_find(SPOIndex *index, size_t subject_id);
int spo_add(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id);
int spo_contains(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id);
int spo_remove(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id);
PredMap *spo_get_preds(SPOIndex *index, size_t subject_id);
IntSet *spo_get_objects(SPOIndex *index, size_t subject_id, size_t predicate_id);
void spo_free(SPOIndex *index);

#endif

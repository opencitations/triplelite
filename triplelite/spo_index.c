/* SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 * SPDX-License-Identifier: ISC */

#include "spo_index.h"

/* --- PredMap ops --- */

static int pm_is_empty(const void *slot)
{
    return ((const PredSlot *)slot)->pred_id == SPO_EMPTY;
}

static void pm_set_empty(void *slot)
{
    ((PredSlot *)slot)->pred_id = SPO_EMPTY;
}

static size_t pm_hash(const void *slot)
{
    return ((const PredSlot *)slot)->pred_id;
}

static int pm_equal(const void *a, const void *b)
{
    return ((const PredSlot *)a)->pred_id == ((const PredSlot *)b)->pred_id;
}

static void pm_cleanup(void *slot)
{
    intset_free(&((PredSlot *)slot)->objects);
}

static const OAOps predmap_ops = {
    sizeof(PredSlot), pm_is_empty, pm_set_empty, pm_hash, pm_equal, pm_cleanup
};

/* --- SPOIndex ops --- */

static int si_is_empty(const void *slot)
{
    return ((const SubjSlot *)slot)->subj_id == SPO_EMPTY;
}

static void si_set_empty(void *slot)
{
    ((SubjSlot *)slot)->subj_id = SPO_EMPTY;
}

static size_t si_hash(const void *slot)
{
    return ((const SubjSlot *)slot)->subj_id;
}

static int si_equal(const void *a, const void *b)
{
    return ((const SubjSlot *)a)->subj_id == ((const SubjSlot *)b)->subj_id;
}

static void si_cleanup(void *slot)
{
    oa_free(&((SubjSlot *)slot)->predicates, &predmap_ops);
}

static const OAOps spo_ops = {
    sizeof(SubjSlot), si_is_empty, si_set_empty, si_hash, si_equal, si_cleanup
};

/* --- Public API --- */

int spo_init(SPOIndex *index, size_t n_slots)
{
    return oa_init(index, n_slots, &spo_ops);
}

SubjSlot *spo_find(SPOIndex *index, size_t subject_id)
{
    SubjSlot key = {subject_id, {NULL, 0, 0}};
    return oa_find(index, &key, &spo_ops);
}

static PredSlot *predmap_find(PredMap *pred_map, size_t predicate_id)
{
    PredSlot key = {predicate_id, {NULL, 0, 0}};
    return oa_find(pred_map, &key, &predmap_ops);
}

static PredSlot *predmap_get_or_create(PredMap *pred_map, size_t predicate_id)
{
    if (pred_map->len * 4 >= pred_map->n_slots * 3) {
        if (oa_grow(pred_map, &predmap_ops) < 0)
            return NULL;
    }
    PredSlot key = {predicate_id, {NULL, 0, 0}};
    size_t idx = oa_probe(pred_map, &key, &predmap_ops);
    PredSlot *slot = (PredSlot *)pred_map->slots + idx;
    if (slot->pred_id != SPO_EMPTY)
        return slot;
    slot->pred_id = predicate_id;
    if (intset_init(&slot->objects, 4) < 0) {
        slot->pred_id = SPO_EMPTY;
        return NULL;
    }
    pred_map->len++;
    return slot;
}

static SubjSlot *spo_get_or_create(SPOIndex *index, size_t subject_id)
{
    if (index->len * 4 >= index->n_slots * 3) {
        if (oa_grow(index, &spo_ops) < 0)
            return NULL;
    }
    SubjSlot key = {subject_id, {NULL, 0, 0}};
    size_t idx = oa_probe(index, &key, &spo_ops);
    SubjSlot *slot = (SubjSlot *)index->slots + idx;
    if (slot->subj_id != SPO_EMPTY)
        return slot;
    slot->subj_id = subject_id;
    if (oa_init(&slot->predicates, 4, &predmap_ops) < 0) {
        slot->subj_id = SPO_EMPTY;
        return NULL;
    }
    index->len++;
    return slot;
}

int spo_add(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id)
{
    SubjSlot *subject = spo_get_or_create(index, subject_id);
    if (subject == NULL)
        return -1;
    PredSlot *predicate = predmap_get_or_create(&subject->predicates, predicate_id);
    if (predicate == NULL)
        return -1;
    return intset_add(&predicate->objects, object_id);
}

int spo_contains(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id)
{
    SubjSlot *subject = spo_find(index, subject_id);
    if (subject == NULL)
        return 0;
    PredSlot *predicate = predmap_find(&subject->predicates, predicate_id);
    if (predicate == NULL)
        return 0;
    return intset_contains(&predicate->objects, object_id);
}

int spo_remove(SPOIndex *index, size_t subject_id, size_t predicate_id, size_t object_id)
{
    SubjSlot *subject = spo_find(index, subject_id);
    if (subject == NULL)
        return 0;
    PredSlot *predicate = predmap_find(&subject->predicates, predicate_id);
    if (predicate == NULL)
        return 0;
    if (!intset_contains(&predicate->objects, object_id))
        return 0;
    intset_remove(&predicate->objects, object_id);
    if (predicate->objects.len == 0) {
        PredSlot pk = {predicate_id, {NULL, 0, 0}};
        size_t pidx = oa_probe(&subject->predicates, &pk, &predmap_ops);
        oa_remove_at(&subject->predicates, pidx, &predmap_ops);
        if (subject->predicates.len == 0) {
            SubjSlot sk = {subject_id, {NULL, 0, 0}};
            size_t sidx = oa_probe(index, &sk, &spo_ops);
            oa_remove_at(index, sidx, &spo_ops);
        }
    }
    return 1;
}

PredMap *spo_get_preds(SPOIndex *index, size_t subject_id)
{
    SubjSlot *subject = spo_find(index, subject_id);
    if (subject == NULL)
        return NULL;
    return &subject->predicates;
}

IntSet *spo_get_objects(SPOIndex *index, size_t subject_id, size_t predicate_id)
{
    SubjSlot *subject = spo_find(index, subject_id);
    if (subject == NULL)
        return NULL;
    PredSlot *predicate = predmap_find(&subject->predicates, predicate_id);
    if (predicate == NULL)
        return NULL;
    return &predicate->objects;
}

void spo_free(SPOIndex *index)
{
    oa_free(index, &spo_ops);
}

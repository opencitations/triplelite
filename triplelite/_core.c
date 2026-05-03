/*
 * SPDX-FileCopyrightText: 2026 Arcangelo Massari <arcangelo.massari@unibo.it>
 *
 * SPDX-License-Identifier: ISC
 */

#include <Python.h>
#if PY_VERSION_HEX < 0x030C0000
#include <structmember.h>
#endif
#include <stddef.h>
#include "dynarray.h"
#include "hashmap.h"
#include "rdfterm_hashmap.h"
#include "spo_index.h"

static PyObject *RDFTermType = NULL;

typedef struct {
    HashMap str_to_id;
    StringArray id_to_str;
} StringInterner;

typedef struct {
    RDFTermHashMap rdfterm_to_id;
    RDFTermArray id_to_rdfterm;
} RDFTermInterner;

static int string_interner_init(StringInterner *interner)
{
    if (hashmap_init(&interner->str_to_id, 16) < 0) {
        return -1;
    }
    string_array_init(&interner->id_to_str);
    return 0;
}

static int string_interner_get_or_create(StringInterner *interner, const char *str, size_t *out) {
    if (hashmap_get(&interner->str_to_id, str, out) == 1) {
        return 0;
    }
    size_t new_id = interner->id_to_str.len;
    if (string_array_append(&interner->id_to_str, str) == -1) {
        return -1;
    }
    if (hashmap_put(&interner->str_to_id, interner->id_to_str.items[new_id], new_id) == -1) {
        return -1;
    }
    *out = new_id;
    return 0;
}

static int string_interner_get(StringInterner *interner, const char *str, size_t *out)
{
    return hashmap_get(&interner->str_to_id, str, out);
}

static int rdfterm_interner_init(RDFTermInterner *interner)
{
    if (rdfterm_hashmap_init(&interner->rdfterm_to_id, 16) == -1) {
        return -1;
    }
    rdfterm_array_init(&interner->id_to_rdfterm);
    return 0;
}

static int rdfterm_interner_get_or_create(RDFTermInterner *interner, StringInterner *strings,
                                          const char *type, const char *value,
                                          const char *datatype, const char *lang, size_t *out)
{
    size_t type_id, value_id, datatype_id, lang_id;
    if (string_interner_get_or_create(strings, type, &type_id) < 0 ||
        string_interner_get_or_create(strings, value, &value_id) < 0 ||
        string_interner_get_or_create(strings, datatype, &datatype_id) < 0 ||
        string_interner_get_or_create(strings, lang, &lang_id) < 0) {
        return -1;
    }
    RDFTerm term = {type_id, value_id, datatype_id, lang_id};
    if (rdfterm_hashmap_get(&interner->rdfterm_to_id, &term, out) == 1) {
        return 0;
    }
    size_t new_id = interner->id_to_rdfterm.len;
    if (rdfterm_array_append(&interner->id_to_rdfterm, &term) == -1 ||
        rdfterm_hashmap_put(&interner->rdfterm_to_id, &term, new_id) == -1) {
        return -1;
    }
    *out = new_id;
    return 0;
}

static int rdfterm_interner_get(RDFTermInterner *interner, StringInterner *strings,
                                const char *type, const char *value,
                                const char *datatype, const char *lang, size_t *out)
{
    size_t type_id, value_id, datatype_id, lang_id;
    if (!string_interner_get(strings, type, &type_id) ||
        !string_interner_get(strings, value, &value_id) ||
        !string_interner_get(strings, datatype, &datatype_id) ||
        !string_interner_get(strings, lang, &lang_id)) {
        return 0;
    }
    RDFTerm term = {type_id, value_id, datatype_id, lang_id};
    return rdfterm_hashmap_get(&interner->rdfterm_to_id, &term, out);
}

static void string_interner_free(StringInterner *interner)
{
    hashmap_free(&interner->str_to_id);
    string_array_free(&interner->id_to_str);
}

static void rdfterm_interner_free(RDFTermInterner *interner)
{
    rdfterm_hashmap_free(&interner->rdfterm_to_id);
    rdfterm_array_free(&interner->id_to_rdfterm);
}

static PyObject *make_rdfterm(StringInterner *strings, RDFTerm *term)
{
    const char *type = strings->id_to_str.items[term->type_id];
    const char *value = strings->id_to_str.items[term->value_id];
    const char *datatype = strings->id_to_str.items[term->datatype_id];
    const char *lang = strings->id_to_str.items[term->lang_id];
    return PyObject_CallFunction(RDFTermType, "ssss", type, value, datatype, lang);
}

#define NO_FILTER ((size_t)-1)

typedef struct {
    PyObject_HEAD
    StringInterner strings;
    RDFTermInterner terms;
    SPOIndex spo;
    SPOIndex pos;
    IntSet indexed_preds;
    int pos_enabled;
    int index_all_preds;
    PyObject *identifier;
    Py_ssize_t len;
} TripleLiteObject;

typedef struct {
    PyObject_HEAD
    TripleLiteObject *store;
    size_t filter_subject_id;
    size_t filter_predicate_id;
    size_t filter_object_id;
    size_t subj_slot;
    size_t pred_slot;
    size_t obj_slot;
    int exhausted;
    int single_subject;
} TripleLiteIterObject;

static PyTypeObject TripleLiteIterType;

static PyObject *make_triple(TripleLiteObject *store, size_t subject_id, size_t predicate_id, size_t object_id)
{
    const char *subject_str = store->strings.id_to_str.items[subject_id];
    const char *predicate_str = store->strings.id_to_str.items[predicate_id];
    RDFTerm *object_term = &store->terms.id_to_rdfterm.items[object_id];
    PyObject *term_tuple = make_rdfterm(&store->strings, object_term);
    if (term_tuple == NULL) {
        return NULL;
    }
    PyObject *triple = Py_BuildValue("(ssN)", subject_str, predicate_str, term_tuple);
    return triple;
}

static PyObject *create_iterator(TripleLiteObject *store, size_t subject_id, size_t predicate_id, size_t object_id, int exhausted)
{
    TripleLiteIterObject *iter = PyObject_New(TripleLiteIterObject, &TripleLiteIterType);
    if (iter == NULL) {
        return NULL;
    }
    Py_INCREF(store);
    iter->store = store;
    iter->filter_subject_id = subject_id;
    iter->filter_predicate_id = predicate_id;
    iter->filter_object_id = object_id;
    iter->pred_slot = 0;
    iter->obj_slot = 0;
    iter->single_subject = 0;

    if (!exhausted && subject_id != NO_FILTER) {
        SubjSlot *entry = spo_find(&store->spo, subject_id);
        if (entry == NULL) {
            iter->exhausted = 1;
        } else {
            iter->subj_slot = (size_t)(entry - (SubjSlot *)store->spo.slots);
            iter->single_subject = 1;
            iter->exhausted = 0;
        }
    } else {
        iter->subj_slot = 0;
        iter->exhausted = exhausted;
    }
    return (PyObject *)iter;
}

static void TripleLiteIter_dealloc(TripleLiteIterObject *self)
{
    Py_DECREF(self->store);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *TripleLiteIter_iter(TripleLiteIterObject *self)
{
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *TripleLiteIter_next(TripleLiteIterObject *self)
{
    if (self->exhausted)
        return NULL;

    SPOIndex *spo = &self->store->spo;
    SubjSlot *subj_slots = (SubjSlot *)spo->slots;
    size_t subj_pos = self->subj_slot;
    size_t pred_pos = self->pred_slot;
    size_t obj_pos = self->obj_slot;

    for (;;) {
        if (self->single_subject) {
            if (subj_slots[subj_pos].subj_id == SPO_EMPTY) {
                self->exhausted = 1;
                return NULL;
            }
        } else {
            while (subj_pos < spo->n_slots && subj_slots[subj_pos].subj_id == SPO_EMPTY)
                subj_pos++;
            if (subj_pos >= spo->n_slots) {
                self->exhausted = 1;
                return NULL;
            }
        }

        SubjSlot *subject = &subj_slots[subj_pos];
        PredMap *pred_map = &subject->predicates;
        PredSlot *pred_slots = (PredSlot *)pred_map->slots;

        while (pred_pos < pred_map->n_slots) {
            if (pred_slots[pred_pos].pred_id != SPO_EMPTY) {
                PredSlot *pred = &pred_slots[pred_pos];
                if (self->filter_predicate_id == NO_FILTER || pred->pred_id == self->filter_predicate_id) {
                    IntSet *obj_set = &pred->objects;
                    IntSetSlot *obj_slots = (IntSetSlot *)obj_set->slots;
                    while (obj_pos < obj_set->n_slots) {
                        if (obj_slots[obj_pos].value != INTSET_EMPTY) {
                            size_t object_id = obj_slots[obj_pos].value;
                            if (self->filter_object_id == NO_FILTER || object_id == self->filter_object_id) {
                                self->subj_slot = subj_pos;
                                self->pred_slot = pred_pos;
                                self->obj_slot = obj_pos + 1;
                                return make_triple(self->store, subject->subj_id, pred->pred_id, object_id);
                            }
                        }
                        obj_pos++;
                    }
                }
            }
            pred_pos++;
            obj_pos = 0;
        }

        if (self->single_subject) {
            self->exhausted = 1;
            return NULL;
        }
        subj_pos++;
        pred_pos = 0;
        obj_pos = 0;
    }
}

static PyTypeObject TripleLiteIterType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "triplelite._core.TripleLiteIterator",
    .tp_basicsize = sizeof(TripleLiteIterObject),
    .tp_dealloc = (destructor)TripleLiteIter_dealloc,
    .tp_iter = (getiterfunc)TripleLiteIter_iter,
    .tp_iternext = (iternextfunc)TripleLiteIter_next,
};

static void TripleLite_dealloc(TripleLiteObject *self)
{
    spo_free(&self->spo);
    if (self->pos_enabled) {
        spo_free(&self->pos);
        if (!self->index_all_preds) {
            intset_free(&self->indexed_preds);
        }
    }
    string_interner_free(&self->strings);
    rdfterm_interner_free(&self->terms);
    Py_XDECREF(self->identifier);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int TripleLite_init(TripleLiteObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"identifier", "reverse_index_predicates", NULL};
    PyObject *identifier = Py_None;
    PyObject *reverse_index_predicates = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist,
                                     &identifier, &reverse_index_predicates)) {
        return -1;
    }

    if (string_interner_init(&self->strings) < 0 ||
        rdfterm_interner_init(&self->terms) < 0 ||
        spo_init(&self->spo, 16) < 0) {
        PyErr_SetString(PyExc_MemoryError, "Failed to initialize TripleLite");
        return -1;
    }

    Py_INCREF(identifier);
    self->identifier = identifier;
    self->len = 0;
    self->pos_enabled = 0;
    self->index_all_preds = 0;

    if (reverse_index_predicates != Py_None) {
        self->pos_enabled = 1;
        if (spo_init(&self->pos, 16) < 0) {
            PyErr_SetString(PyExc_MemoryError, "Failed to initialize POS index");
            return -1;
        }
        Py_ssize_t size = PyObject_Length(reverse_index_predicates);
        if (size < 0) return -1;
        if (size == 0) {
            self->index_all_preds = 1;
        } else {
            self->index_all_preds = 0;
            if (intset_init(&self->indexed_preds, (size_t)size * 2) < 0) {
                PyErr_SetString(PyExc_MemoryError, "Failed to initialize indexed predicates");
                return -1;
            }
            PyObject *iter = PyObject_GetIter(reverse_index_predicates);
            if (iter == NULL) return -1;
            PyObject *item;
            while ((item = PyIter_Next(iter)) != NULL) {
                const char *pred_str = PyUnicode_AsUTF8(item);
                if (pred_str == NULL) {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    return -1;
                }
                size_t pred_id;
                if (string_interner_get_or_create(&self->strings, pred_str, &pred_id) < 0) {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    PyErr_SetString(PyExc_MemoryError, "Failed to intern predicate");
                    return -1;
                }
                Py_DECREF(item);
                if (intset_add(&self->indexed_preds, pred_id) < 0) {
                    Py_DECREF(iter);
                    return -1;
                }
            }
            Py_DECREF(iter);
            if (PyErr_Occurred()) return -1;
        }
    }

    return 0;
}

static PyObject *TripleLite_add(TripleLiteObject *self, PyObject *args)
{
    const char *subject;
    const char *predicate;
    const char *type;
    const char *value;
    const char *datatype;
    const char *lang;

    if (!PyArg_ParseTuple(args, "(ss(ssss))", &subject, &predicate,
                          &type, &value, &datatype, &lang)) {
        return NULL;
    }

    size_t subject_id, predicate_id, object_id;
    if (string_interner_get_or_create(&self->strings, subject, &subject_id) < 0 ||
        string_interner_get_or_create(&self->strings, predicate, &predicate_id) < 0) {
        return PyErr_NoMemory();
    }

    if (rdfterm_interner_get_or_create(&self->terms, &self->strings,
                                       type, value, datatype, lang, &object_id) < 0) {
        return PyErr_NoMemory();
    }

    int added = spo_add(&self->spo, subject_id, predicate_id, object_id);
    if (added < 0) {
        return PyErr_NoMemory();
    }
    if (added == 1) {
        self->len++;
        if (self->pos_enabled) {
            if (self->index_all_preds || intset_contains(&self->indexed_preds, predicate_id)) {
                if (spo_add(&self->pos, predicate_id, object_id, subject_id) < 0) {
                    return PyErr_NoMemory();
                }
            }
        }
    }

    Py_RETURN_NONE;
}

static int add_triple_from_pyobject(TripleLiteObject *self, PyObject *item)
{
    const char *subject, *predicate, *type, *value, *datatype, *lang;
    if (!PyArg_ParseTuple(item, "ss(ssss)", &subject, &predicate,
                          &type, &value, &datatype, &lang)) {
        return -1;
    }

    size_t subject_id, predicate_id, object_id;
    if (string_interner_get_or_create(&self->strings, subject, &subject_id) < 0 ||
        string_interner_get_or_create(&self->strings, predicate, &predicate_id) < 0) {
        PyErr_NoMemory();
        return -1;
    }

    if (rdfterm_interner_get_or_create(&self->terms, &self->strings,
                                       type, value, datatype, lang, &object_id) < 0) {
        PyErr_NoMemory();
        return -1;
    }

    int added = spo_add(&self->spo, subject_id, predicate_id, object_id);
    if (added < 0) {
        PyErr_NoMemory();
        return -1;
    }
    if (added == 1) {
        self->len++;
        if (self->pos_enabled) {
            if (self->index_all_preds || intset_contains(&self->indexed_preds, predicate_id)) {
                if (spo_add(&self->pos, predicate_id, object_id, subject_id) < 0) {
                    PyErr_NoMemory();
                    return -1;
                }
            }
        }
    }
    return 0;
}

static PyObject *TripleLite_add_many(TripleLiteObject *self, PyObject *args)
{
    PyObject *iterable;
    if (!PyArg_ParseTuple(args, "O", &iterable)) {
        return NULL;
    }

    PyObject *iter = PyObject_GetIter(iterable);
    if (iter == NULL) return NULL;

    PyObject *item;
    while ((item = PyIter_Next(iter)) != NULL) {
        int result = add_triple_from_pyobject(self, item);
        Py_DECREF(item);
        if (result < 0) {
            Py_DECREF(iter);
            return NULL;
        }
    }
    Py_DECREF(iter);
    if (PyErr_Occurred()) return NULL;

    Py_RETURN_NONE;
}

static PyObject *TripleLite_iter(TripleLiteObject *self)
{
    return create_iterator(self, NO_FILTER, NO_FILTER, NO_FILTER, 0);
}

static int resolve_filter(StringInterner *string_interner, RDFTermInterner *term_interner,
                          PyObject *py_subject, PyObject *py_predicate, PyObject *py_object,
                          size_t *subject_id, size_t *predicate_id, size_t *object_id, int *empty)
{
    *subject_id = NO_FILTER;
    *predicate_id = NO_FILTER;
    *object_id = NO_FILTER;
    *empty = 0;

    if (py_subject != Py_None) {
        const char *subject_str = PyUnicode_AsUTF8(py_subject);
        if (subject_str == NULL) return -1;
        if (!string_interner_get(string_interner, subject_str, subject_id)) { *empty = 1; return 0; }
    }
    if (py_predicate != Py_None) {
        const char *predicate_str = PyUnicode_AsUTF8(py_predicate);
        if (predicate_str == NULL) return -1;
        if (!string_interner_get(string_interner, predicate_str, predicate_id)) { *empty = 1; return 0; }
    }
    if (py_object != Py_None) {
        const char *type, *value, *datatype, *lang;
        if (!PyArg_ParseTuple(py_object, "ssss", &type, &value, &datatype, &lang)) return -1;
        if (!rdfterm_interner_get(term_interner, string_interner, type, value, datatype, lang, object_id)) { *empty = 1; return 0; }
    }
    return 0;
}

static PyObject *TripleLite_triples(TripleLiteObject *self, PyObject *args)
{
    PyObject *py_subject, *py_predicate, *py_object;
    if (!PyArg_ParseTuple(args, "(OOO)", &py_subject, &py_predicate, &py_object)) {
        return NULL;
    }

    size_t subject_id, predicate_id, object_id;
    int empty;
    if (resolve_filter(&self->strings, &self->terms, py_subject, py_predicate, py_object,
                       &subject_id, &predicate_id, &object_id, &empty) < 0) {
        return NULL;
    }
    return create_iterator(self, subject_id, predicate_id, object_id, empty);
}

static PyObject *TripleLite_objects(TripleLiteObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"subject", "predicate", NULL};
    PyObject *py_subject = Py_None;
    PyObject *py_predicate = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &py_subject, &py_predicate)) {
        return NULL;
    }

    size_t subject_id, predicate_id, object_id;
    int empty;
    if (resolve_filter(&self->strings, &self->terms, py_subject, py_predicate, Py_None,
                       &subject_id, &predicate_id, &object_id, &empty) < 0) {
        return NULL;
    }

    if (empty) {
        return PyList_New(0);
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        return NULL;
    }

    SPOIndex *spo = &self->spo;
    RDFTermArray *terms = &self->terms.id_to_rdfterm;

    if (subject_id != NO_FILTER && predicate_id != NO_FILTER) {
        IntSet *object_set = spo_get_objects(spo, subject_id, predicate_id);
        if (object_set != NULL) {
            IntSetSlot *obj_slots = (IntSetSlot *)object_set->slots;
            for (size_t i = 0; i < object_set->n_slots; i++) {
                if (obj_slots[i].value == INTSET_EMPTY) continue;
                PyObject *term = make_rdfterm(&self->strings, &terms->items[obj_slots[i].value]);
                if (term == NULL || PyList_Append(result, term) < 0) {
                    Py_XDECREF(term);
                    Py_DECREF(result);
                    return NULL;
                }
                Py_DECREF(term);
            }
        }
    } else if (subject_id != NO_FILTER) {
        PredMap *pred_map = spo_get_preds(spo, subject_id);
        if (pred_map != NULL) {
            PredSlot *pred_slots = (PredSlot *)pred_map->slots;
            for (size_t i = 0; i < pred_map->n_slots; i++) {
                if (pred_slots[i].pred_id == SPO_EMPTY) continue;
                IntSetSlot *obj_slots = (IntSetSlot *)pred_slots[i].objects.slots;
                for (size_t j = 0; j < pred_slots[i].objects.n_slots; j++) {
                    if (obj_slots[j].value == INTSET_EMPTY) continue;
                    PyObject *term = make_rdfterm(&self->strings, &terms->items[obj_slots[j].value]);
                    if (term == NULL || PyList_Append(result, term) < 0) {
                        Py_XDECREF(term);
                        Py_DECREF(result);
                        return NULL;
                    }
                    Py_DECREF(term);
                }
            }
        }
    } else {
        SubjSlot *subj_slots = (SubjSlot *)spo->slots;
        for (size_t i = 0; i < spo->n_slots; i++) {
            if (subj_slots[i].subj_id == SPO_EMPTY) continue;
            PredSlot *pred_slots = (PredSlot *)subj_slots[i].predicates.slots;
            for (size_t j = 0; j < subj_slots[i].predicates.n_slots; j++) {
                if (pred_slots[j].pred_id == SPO_EMPTY) continue;
                if (predicate_id != NO_FILTER && pred_slots[j].pred_id != predicate_id) continue;
                IntSetSlot *obj_slots = (IntSetSlot *)pred_slots[j].objects.slots;
                for (size_t k = 0; k < pred_slots[j].objects.n_slots; k++) {
                    if (obj_slots[k].value == INTSET_EMPTY) continue;
                    PyObject *term = make_rdfterm(&self->strings, &terms->items[obj_slots[k].value]);
                    if (term == NULL || PyList_Append(result, term) < 0) {
                        Py_XDECREF(term);
                        Py_DECREF(result);
                        return NULL;
                    }
                    Py_DECREF(term);
                }
            }
        }
    }

    PyObject *iter = PyObject_GetIter(result);
    Py_DECREF(result);
    return iter;
}

static PyObject *TripleLite_predicate_objects(TripleLiteObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"subject", NULL};
    PyObject *py_subject = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &py_subject)) {
        return NULL;
    }

    size_t subject_id = NO_FILTER;
    if (py_subject != Py_None) {
        const char *subject_str = PyUnicode_AsUTF8(py_subject);
        if (subject_str == NULL) return NULL;
        if (!string_interner_get(&self->strings, subject_str, &subject_id)) {
            return PyList_New(0);
        }
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) return NULL;

    SPOIndex *spo = &self->spo;
    StringArray *strings = &self->strings.id_to_str;
    RDFTermArray *terms = &self->terms.id_to_rdfterm;

    if (subject_id != NO_FILTER) {
        PredMap *pred_map = spo_get_preds(spo, subject_id);
        if (pred_map != NULL) {
            PredSlot *pred_slots = (PredSlot *)pred_map->slots;
            for (size_t i = 0; i < pred_map->n_slots; i++) {
                if (pred_slots[i].pred_id == SPO_EMPTY) continue;
                const char *predicate_str = strings->items[pred_slots[i].pred_id];
                IntSetSlot *obj_slots = (IntSetSlot *)pred_slots[i].objects.slots;
                for (size_t j = 0; j < pred_slots[i].objects.n_slots; j++) {
                    if (obj_slots[j].value == INTSET_EMPTY) continue;
                    PyObject *term = make_rdfterm(&self->strings, &terms->items[obj_slots[j].value]);
                    if (term == NULL) {
                        Py_DECREF(result);
                        return NULL;
                    }
                    PyObject *pair = Py_BuildValue("(sN)", predicate_str, term);
                    if (pair == NULL || PyList_Append(result, pair) < 0) {
                        Py_XDECREF(pair);
                        Py_DECREF(result);
                        return NULL;
                    }
                    Py_DECREF(pair);
                }
            }
        }
    } else {
        SubjSlot *subj_slots = (SubjSlot *)spo->slots;
        for (size_t i = 0; i < spo->n_slots; i++) {
            if (subj_slots[i].subj_id == SPO_EMPTY) continue;
            PredSlot *pred_slots = (PredSlot *)subj_slots[i].predicates.slots;
            for (size_t j = 0; j < subj_slots[i].predicates.n_slots; j++) {
                if (pred_slots[j].pred_id == SPO_EMPTY) continue;
                const char *predicate_str = strings->items[pred_slots[j].pred_id];
                IntSetSlot *obj_slots = (IntSetSlot *)pred_slots[j].objects.slots;
                for (size_t k = 0; k < pred_slots[j].objects.n_slots; k++) {
                    if (obj_slots[k].value == INTSET_EMPTY) continue;
                    PyObject *term = make_rdfterm(&self->strings, &terms->items[obj_slots[k].value]);
                    if (term == NULL) {
                        Py_DECREF(result);
                        return NULL;
                    }
                    PyObject *pair = Py_BuildValue("(sN)", predicate_str, term);
                    if (pair == NULL || PyList_Append(result, pair) < 0) {
                        Py_XDECREF(pair);
                        Py_DECREF(result);
                        return NULL;
                    }
                    Py_DECREF(pair);
                }
            }
        }
    }

    PyObject *iter = PyObject_GetIter(result);
    Py_DECREF(result);
    return iter;
}

static PyObject *TripleLite_subjects(TripleLiteObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"predicate", "object", NULL};
    PyObject *py_predicate = Py_None;
    PyObject *py_object = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", kwlist, &py_predicate, &py_object)) {
        return NULL;
    }

    size_t predicate_id = NO_FILTER, object_id = NO_FILTER;
    if (py_predicate != Py_None) {
        const char *predicate_str = PyUnicode_AsUTF8(py_predicate);
        if (predicate_str == NULL) return NULL;
        if (!string_interner_get(&self->strings, predicate_str, &predicate_id)) {
            return PyList_New(0);
        }
    }
    if (py_object != Py_None) {
        const char *type, *value, *datatype, *lang;
        if (!PyArg_ParseTuple(py_object, "ssss", &type, &value, &datatype, &lang)) return NULL;
        if (!rdfterm_interner_get(&self->terms, &self->strings, type, value, datatype, lang, &object_id)) {
            return PyList_New(0);
        }
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) return NULL;

    StringArray *strings = &self->strings.id_to_str;

    if (self->pos_enabled) {
        SPOIndex *pos = &self->pos;

        if (predicate_id != NO_FILTER && object_id != NO_FILTER) {
            IntSet *subjects = spo_get_objects(pos, predicate_id, object_id);
            if (subjects != NULL) {
                IntSetSlot *subject_slots = (IntSetSlot *)subjects->slots;
                for (size_t i = 0; i < subjects->n_slots; i++) {
                    if (subject_slots[i].value == INTSET_EMPTY) continue;
                    PyObject *py_str = PyUnicode_FromString(strings->items[subject_slots[i].value]);
                    if (py_str == NULL || PyList_Append(result, py_str) < 0) {
                        Py_XDECREF(py_str);
                        Py_DECREF(result);
                        return NULL;
                    }
                    Py_DECREF(py_str);
                }
            }
        } else if (predicate_id != NO_FILTER) {
            PredMap *obj_map = spo_get_preds(pos, predicate_id);
            if (obj_map != NULL) {
                PredSlot *obj_entries = (PredSlot *)obj_map->slots;
                for (size_t i = 0; i < obj_map->n_slots; i++) {
                    if (obj_entries[i].pred_id == SPO_EMPTY) continue;
                    IntSetSlot *subject_slots = (IntSetSlot *)obj_entries[i].objects.slots;
                    for (size_t j = 0; j < obj_entries[i].objects.n_slots; j++) {
                        if (subject_slots[j].value == INTSET_EMPTY) continue;
                        PyObject *py_str = PyUnicode_FromString(strings->items[subject_slots[j].value]);
                        if (py_str == NULL || PyList_Append(result, py_str) < 0) {
                            Py_XDECREF(py_str);
                            Py_DECREF(result);
                            return NULL;
                        }
                        Py_DECREF(py_str);
                    }
                }
            }
        } else if (object_id != NO_FILTER) {
            SubjSlot *pos_slots = (SubjSlot *)pos->slots;
            for (size_t i = 0; i < pos->n_slots; i++) {
                if (pos_slots[i].subj_id == SPO_EMPTY) continue;
                IntSet *subjects = spo_get_objects(pos, pos_slots[i].subj_id, object_id);
                if (subjects == NULL) continue;
                IntSetSlot *subject_slots = (IntSetSlot *)subjects->slots;
                for (size_t j = 0; j < subjects->n_slots; j++) {
                    if (subject_slots[j].value == INTSET_EMPTY) continue;
                    PyObject *py_str = PyUnicode_FromString(strings->items[subject_slots[j].value]);
                    if (py_str == NULL || PyList_Append(result, py_str) < 0) {
                        Py_XDECREF(py_str);
                        Py_DECREF(result);
                        return NULL;
                    }
                    Py_DECREF(py_str);
                }
            }
        } else {
            IntSet seen;
            if (intset_init(&seen, 16) < 0) {
                Py_DECREF(result);
                return PyErr_NoMemory();
            }
            SubjSlot *pos_slots = (SubjSlot *)pos->slots;
            for (size_t i = 0; i < pos->n_slots; i++) {
                if (pos_slots[i].subj_id == SPO_EMPTY) continue;
                PredSlot *obj_entries = (PredSlot *)pos_slots[i].predicates.slots;
                for (size_t j = 0; j < pos_slots[i].predicates.n_slots; j++) {
                    if (obj_entries[j].pred_id == SPO_EMPTY) continue;
                    IntSetSlot *subject_slots = (IntSetSlot *)obj_entries[j].objects.slots;
                    for (size_t k = 0; k < obj_entries[j].objects.n_slots; k++) {
                        if (subject_slots[k].value == INTSET_EMPTY) continue;
                        size_t subject_id = subject_slots[k].value;
                        if (intset_contains(&seen, subject_id)) continue;
                        if (intset_add(&seen, subject_id) < 0) {
                            intset_free(&seen);
                            Py_DECREF(result);
                            return PyErr_NoMemory();
                        }
                        PyObject *py_str = PyUnicode_FromString(strings->items[subject_id]);
                        if (py_str == NULL || PyList_Append(result, py_str) < 0) {
                            Py_XDECREF(py_str);
                            intset_free(&seen);
                            Py_DECREF(result);
                            return NULL;
                        }
                        Py_DECREF(py_str);
                    }
                }
            }
            intset_free(&seen);
        }
    } else {
        SPOIndex *spo = &self->spo;
        SubjSlot *subj_slots = (SubjSlot *)spo->slots;
        for (size_t i = 0; i < spo->n_slots; i++) {
            if (subj_slots[i].subj_id == SPO_EMPTY) continue;
            PredSlot *pred_slots = (PredSlot *)subj_slots[i].predicates.slots;
            int found = 0;
            for (size_t j = 0; j < subj_slots[i].predicates.n_slots && !found; j++) {
                if (pred_slots[j].pred_id == SPO_EMPTY) continue;
                if (predicate_id != NO_FILTER && pred_slots[j].pred_id != predicate_id) continue;
                if (object_id != NO_FILTER) {
                    if (intset_contains(&pred_slots[j].objects, object_id)) found = 1;
                } else {
                    if (pred_slots[j].objects.len > 0) found = 1;
                }
            }
            if (found) {
                const char *subject_str = strings->items[subj_slots[i].subj_id];
                PyObject *py_str = PyUnicode_FromString(subject_str);
                if (py_str == NULL || PyList_Append(result, py_str) < 0) {
                    Py_XDECREF(py_str);
                    Py_DECREF(result);
                    return NULL;
                }
                Py_DECREF(py_str);
            }
        }
    }

    PyObject *iter = PyObject_GetIter(result);
    Py_DECREF(result);
    return iter;
}

static PyObject *TripleLite_remove(TripleLiteObject *self, PyObject *args)
{
    PyObject *py_subject, *py_predicate, *py_object;
    if (!PyArg_ParseTuple(args, "(OOO)", &py_subject, &py_predicate, &py_object)) {
        return NULL;
    }

    if (py_subject == Py_None && py_predicate == Py_None && py_object == Py_None) {
        spo_free(&self->spo);
        if (spo_init(&self->spo, 16) < 0) {
            return PyErr_NoMemory();
        }
        if (self->pos_enabled) {
            spo_free(&self->pos);
            if (spo_init(&self->pos, 16) < 0) {
                return PyErr_NoMemory();
            }
        }
        self->len = 0;
        Py_RETURN_NONE;
    }

    size_t subject_id, predicate_id, object_id;
    int empty;
    if (resolve_filter(&self->strings, &self->terms, py_subject, py_predicate, py_object,
                       &subject_id, &predicate_id, &object_id, &empty) < 0) {
        return NULL;
    }
    if (empty) {
        Py_RETURN_NONE;
    }

    if (subject_id != NO_FILTER && predicate_id != NO_FILTER && object_id != NO_FILTER) {
        if (spo_remove(&self->spo, subject_id, predicate_id, object_id) == 1) {
            self->len--;
            if (self->pos_enabled) {
                spo_remove(&self->pos, predicate_id, object_id, subject_id);
            }
        }
        Py_RETURN_NONE;
    }

    typedef struct { size_t subject_id, predicate_id, object_id; } TripleIds;
    TripleIds *to_remove = NULL;
    size_t remove_count = 0;
    size_t remove_capacity = 0;

    SPOIndex *spo = &self->spo;
    SubjSlot *subj_slots = (SubjSlot *)spo->slots;
    for (size_t i = 0; i < spo->n_slots; i++) {
        if (subj_slots[i].subj_id == SPO_EMPTY) continue;
        if (subject_id != NO_FILTER && subj_slots[i].subj_id != subject_id) continue;
        PredSlot *pred_slots = (PredSlot *)subj_slots[i].predicates.slots;
        for (size_t j = 0; j < subj_slots[i].predicates.n_slots; j++) {
            if (pred_slots[j].pred_id == SPO_EMPTY) continue;
            if (predicate_id != NO_FILTER && pred_slots[j].pred_id != predicate_id) continue;
            IntSetSlot *obj_slots = (IntSetSlot *)pred_slots[j].objects.slots;
            for (size_t k = 0; k < pred_slots[j].objects.n_slots; k++) {
                if (obj_slots[k].value == INTSET_EMPTY) continue;
                if (object_id != NO_FILTER && obj_slots[k].value != object_id) continue;
                if (remove_count >= remove_capacity) {
                    size_t new_capacity = remove_capacity == 0 ? 16 : remove_capacity * 2;
                    TripleIds *new_buf = realloc(to_remove, new_capacity * sizeof(TripleIds));
                    if (new_buf == NULL) {
                        free(to_remove);
                        return PyErr_NoMemory();
                    }
                    to_remove = new_buf;
                    remove_capacity = new_capacity;
                }
                to_remove[remove_count++] = (TripleIds){.subject_id = subj_slots[i].subj_id, .predicate_id = pred_slots[j].pred_id, .object_id = obj_slots[k].value};
            }
        }
    }

    for (size_t i = 0; i < remove_count; i++) {
        if (spo_remove(&self->spo, to_remove[i].subject_id, to_remove[i].predicate_id, to_remove[i].object_id) == 1) {
            self->len--;
            if (self->pos_enabled) {
                spo_remove(&self->pos, to_remove[i].predicate_id, to_remove[i].object_id, to_remove[i].subject_id);
            }
        }
    }
    free(to_remove);

    Py_RETURN_NONE;
}

static int TripleLite_sq_contains(TripleLiteObject *self, PyObject *arg)
{
    const char *subject;
    const char *predicate;
    const char *type;
    const char *value;
    const char *datatype;
    const char *lang;

    if (!PyArg_ParseTuple(arg, "ss(ssss)", &subject, &predicate,
                          &type, &value, &datatype, &lang)) {
        return -1;
    }

    size_t subject_id, predicate_id, object_id;
    if (!string_interner_get(&self->strings, subject, &subject_id) ||
        !string_interner_get(&self->strings, predicate, &predicate_id)) {
        return 0;
    }

    if (!rdfterm_interner_get(&self->terms, &self->strings, type, value, datatype, lang, &object_id)) {
        return 0;
    }

    return spo_contains(&self->spo, subject_id, predicate_id, object_id);
}

static PyObject *TripleLite_has_subject(TripleLiteObject *self, PyObject *args)
{
    const char *subject;
    if (!PyArg_ParseTuple(args, "s", &subject)) {
        return NULL;
    }

    size_t subject_id;
    if (!string_interner_get(&self->strings, subject, &subject_id)) {
        Py_RETURN_FALSE;
    }

    if (spo_get_preds(&self->spo, subject_id) != NULL) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static Py_ssize_t TripleLite_sq_length(TripleLiteObject *self)
{
    return self->len;
}

static PyMethodDef TripleLite_methods[] = {
    {"add", (PyCFunction)TripleLite_add, METH_VARARGS, NULL},
    {"add_many", (PyCFunction)TripleLite_add_many, METH_VARARGS, NULL},
    {"remove", (PyCFunction)TripleLite_remove, METH_VARARGS, NULL},
    {"triples", (PyCFunction)TripleLite_triples, METH_VARARGS, NULL},
    {"objects", (PyCFunction)TripleLite_objects, METH_VARARGS | METH_KEYWORDS, NULL},
    {"predicate_objects", (PyCFunction)TripleLite_predicate_objects, METH_VARARGS | METH_KEYWORDS, NULL},
    {"subjects", (PyCFunction)TripleLite_subjects, METH_VARARGS | METH_KEYWORDS, NULL},
    {"has_subject", (PyCFunction)TripleLite_has_subject, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

static PyMemberDef TripleLite_members[] = {
#if PY_VERSION_HEX >= 0x030C0000
    {"identifier", Py_T_OBJECT_EX, offsetof(TripleLiteObject, identifier), 0, NULL},
#else
    {"identifier", T_OBJECT_EX, offsetof(TripleLiteObject, identifier), 0, NULL},
#endif
    {NULL}
};

static PySequenceMethods TripleLite_as_sequence = {
    .sq_length = (lenfunc)TripleLite_sq_length,
    .sq_contains = (objobjproc)TripleLite_sq_contains,
};

static PyTypeObject TripleLiteType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "triplelite._core.TripleLite",
    .tp_basicsize = sizeof(TripleLiteObject),
    .tp_dealloc = (destructor)TripleLite_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_iter = (getiterfunc)TripleLite_iter,
    .tp_as_sequence = &TripleLite_as_sequence,
    .tp_members = TripleLite_members,
    .tp_methods = TripleLite_methods,
    .tp_init = (initproc)TripleLite_init,
    .tp_new = PyType_GenericNew,
};

static PyMethodDef core_methods[] = {
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef core_module = {
    PyModuleDef_HEAD_INIT,
    "_core",
    "C engine for the TripleLite RDF triple store.",
    -1,
    core_methods
};

PyMODINIT_FUNC
PyInit__core(void)
{
    PyObject *types_module = PyImport_ImportModule("triplelite._types");
    if (types_module == NULL) {
        return NULL;
    }
    RDFTermType = PyObject_GetAttrString(types_module, "RDFTerm");
    Py_DECREF(types_module);
    if (RDFTermType == NULL) {
        return NULL;
    }
    if (PyType_Ready(&TripleLiteIterType) < 0 ||
        PyType_Ready(&TripleLiteType) < 0) {
        return NULL;
    }
    PyObject *module = PyModule_Create(&core_module);
    if (module == NULL) {
        return NULL;
    }
    Py_INCREF(&TripleLiteType);
    if (PyModule_AddObject(module, "TripleLite", (PyObject *)&TripleLiteType) < 0) {
        Py_DECREF(&TripleLiteType);
        Py_DECREF(module);
        return NULL;
    }
    return module;
}

#include <Python.h>
#include "dynarray.h"
#include "hashmap.h"
#include "rdfterm_hashmap.h"
#include "spo_index.h"

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
    if (string_array_append(&interner->id_to_str, str) == -1 || hashmap_put(&interner->str_to_id, str, new_id) == -1) {
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

static int rdfterm_interner_get_or_create(RDFTermInterner *interner, const RDFTerm *term, size_t *out)
{
    if (rdfterm_hashmap_get(&interner->rdfterm_to_id, term, out) == 1) {
        return 0;
    }
    size_t new_id = interner->id_to_rdfterm.len;
    if (rdfterm_array_append(&interner->id_to_rdfterm, term) == -1 ||
        rdfterm_hashmap_put(&interner->rdfterm_to_id, term, new_id) == -1) {
        return -1;
    }
    *out = new_id;
    return 0;
}

static int rdfterm_interner_get(RDFTermInterner *interner, const RDFTerm *term, size_t *out)
{
    return rdfterm_hashmap_get(&interner->rdfterm_to_id, term, out);
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

#define NO_FILTER ((size_t)-1)

typedef struct {
    PyObject_HEAD
    StringInterner strings;
    RDFTermInterner terms;
    SPOIndex spo;
    Py_ssize_t len;
} TripleLiteObject;

typedef struct {
    PyObject_HEAD
    TripleLiteObject *store;
    size_t filter_subject_id;
    size_t filter_predicate_id;
    size_t filter_object_id;
    size_t spo_bucket;
    SubjEntry *subject;
    size_t predicate_bucket;
    PredEntry *predicate;
    size_t object_slot;
    int exhausted;
} TripleLiteIterObject;

static PyTypeObject TripleLiteIterType;

static PyObject *make_triple(TripleLiteObject *store, size_t subject_id, size_t predicate_id, size_t object_id)
{
    const char *subject_str = store->strings.id_to_str.items[subject_id];
    const char *predicate_str = store->strings.id_to_str.items[predicate_id];
    RDFTerm *object_term = &store->terms.id_to_rdfterm.items[object_id];
    PyObject *term_tuple = Py_BuildValue("(ssss)", object_term->type, object_term->value, object_term->datatype, object_term->lang);
    if (term_tuple == NULL) {
        return NULL;
    }
    PyObject *triple = Py_BuildValue("(ssN)", subject_str, predicate_str, term_tuple);
    return triple;
}

static PyObject *create_iterator(TripleLiteObject *store, size_t subject_id, size_t predicate_id, size_t object_id, int exhausted)
{
    TripleLiteIterObject *it = PyObject_New(TripleLiteIterObject, &TripleLiteIterType);
    if (it == NULL) {
        return NULL;
    }
    Py_INCREF(store);
    it->store = store;
    it->filter_subject_id = subject_id;
    it->filter_predicate_id = predicate_id;
    it->filter_object_id = object_id;
    it->spo_bucket = 0;
    it->subject = NULL;
    it->predicate_bucket = 0;
    it->predicate = NULL;
    it->object_slot = 0;
    it->exhausted = exhausted;
    return (PyObject *)it;
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

static PyObject *TripleLiteIter_next(TripleLiteIterObject *it)
{
    if (it->exhausted) {
        return NULL;
    }

    SPOIndex *spo = &it->store->spo;
    SubjEntry *subject = it->subject;
    PredEntry *predicate = it->predicate;
    size_t object_slot = it->object_slot;
    size_t subject_bucket = it->spo_bucket;
    size_t predicate_bucket = it->predicate_bucket;

    for (;;) {
        while (subject == NULL) {
            if (subject_bucket >= spo->n_buckets) {
                it->exhausted = 1;
                return NULL;
            }
            subject = spo->buckets[subject_bucket++];
            predicate = NULL;
            predicate_bucket = 0;
        }

        if (it->filter_subject_id != NO_FILTER && subject->subj_id != it->filter_subject_id) {
            subject = subject->next;
            continue;
        }

        PredMap *pred_map = &subject->predicates;

        for (;;) {
            while (predicate == NULL) {
                if (predicate_bucket >= pred_map->n_buckets) {
                    goto next_subject;
                }
                predicate = pred_map->buckets[predicate_bucket++];
                object_slot = 0;
            }

            if (it->filter_predicate_id != NO_FILTER && predicate->pred_id != it->filter_predicate_id) {
                predicate = predicate->next;
                continue;
            }

            IntSet *object_set = &predicate->objects;

            while (object_slot < object_set->n_slots) {
                if (object_set->occupied[object_slot]) {
                    size_t object_id = object_set->slots[object_slot];
                    if (it->filter_object_id == NO_FILTER || object_id == it->filter_object_id) {
                        it->spo_bucket = subject_bucket;
                        it->subject = subject;
                        it->predicate_bucket = predicate_bucket;
                        it->predicate = predicate;
                        it->object_slot = object_slot + 1;
                        return make_triple(it->store, subject->subj_id, predicate->pred_id, object_id);
                    }
                }
                object_slot++;
            }

            predicate = predicate->next;
            object_slot = 0;
        }

    next_subject:
        subject = subject->next;
        predicate = NULL;
        predicate_bucket = 0;
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
    string_interner_free(&self->strings);
    rdfterm_interner_free(&self->terms);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int TripleLite_init(TripleLiteObject *self, PyObject *args, PyObject *kwds)
{
    if (string_interner_init(&self->strings) < 0 ||
        rdfterm_interner_init(&self->terms) < 0 ||
        spo_init(&self->spo, 16) < 0) {
        PyErr_SetString(PyExc_MemoryError, "Failed to initialize TripleLite");
        return -1;
    }
    self->len = 0;
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

    RDFTerm term = {type, value, datatype, lang};
    if (rdfterm_interner_get_or_create(&self->terms, &term, &object_id) < 0) {
        return PyErr_NoMemory();
    }

    int added = spo_add(&self->spo, subject_id, predicate_id, object_id);
    if (added < 0) {
        return PyErr_NoMemory();
    }
    if (added == 1) {
        self->len++;
    }

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
        RDFTerm term = {type, value, datatype, lang};
        if (!rdfterm_interner_get(term_interner, &term, object_id)) { *empty = 1; return 0; }
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

    for (size_t bucket_idx = 0; bucket_idx < spo->n_buckets; bucket_idx++) {
        for (SubjEntry *subject_entry = spo->buckets[bucket_idx]; subject_entry != NULL; subject_entry = subject_entry->next) {
            if (subject_id != NO_FILTER && subject_entry->subj_id != subject_id) continue;
            PredMap *pred_map = &subject_entry->predicates;
            for (size_t predicate_bucket = 0; predicate_bucket < pred_map->n_buckets; predicate_bucket++) {
                for (PredEntry *predicate_entry = pred_map->buckets[predicate_bucket]; predicate_entry != NULL; predicate_entry = predicate_entry->next) {
                    if (predicate_id != NO_FILTER && predicate_entry->pred_id != predicate_id) continue;
                    IntSet *object_set = &predicate_entry->objects;
                    for (size_t i = 0; i < object_set->n_slots; i++) {
                        if (object_set->occupied[i]) {
                            RDFTerm *object_term = &terms->items[object_set->slots[i]];
                            PyObject *term = Py_BuildValue("(ssss)", object_term->type, object_term->value, object_term->datatype, object_term->lang);
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

    for (size_t bucket_idx = 0; bucket_idx < spo->n_buckets; bucket_idx++) {
        for (SubjEntry *subject_entry = spo->buckets[bucket_idx]; subject_entry != NULL; subject_entry = subject_entry->next) {
            if (subject_id != NO_FILTER && subject_entry->subj_id != subject_id) continue;
            PredMap *pred_map = &subject_entry->predicates;
            for (size_t predicate_bucket = 0; predicate_bucket < pred_map->n_buckets; predicate_bucket++) {
                for (PredEntry *predicate_entry = pred_map->buckets[predicate_bucket]; predicate_entry != NULL; predicate_entry = predicate_entry->next) {
                    const char *predicate_str = strings->items[predicate_entry->pred_id];
                    IntSet *object_set = &predicate_entry->objects;
                    for (size_t i = 0; i < object_set->n_slots; i++) {
                        if (object_set->occupied[i]) {
                            RDFTerm *object_term = &terms->items[object_set->slots[i]];
                            PyObject *pair = Py_BuildValue("(s(ssss))", predicate_str, object_term->type, object_term->value, object_term->datatype, object_term->lang);
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
        RDFTerm term = {type, value, datatype, lang};
        if (!rdfterm_interner_get(&self->terms, &term, &object_id)) {
            return PyList_New(0);
        }
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) return NULL;

    SPOIndex *spo = &self->spo;
    StringArray *strings = &self->strings.id_to_str;

    for (size_t bucket_idx = 0; bucket_idx < spo->n_buckets; bucket_idx++) {
        for (SubjEntry *subject_entry = spo->buckets[bucket_idx]; subject_entry != NULL; subject_entry = subject_entry->next) {
            PredMap *pred_map = &subject_entry->predicates;
            int found = 0;
            for (size_t predicate_bucket = 0; predicate_bucket < pred_map->n_buckets && !found; predicate_bucket++) {
                for (PredEntry *predicate_entry = pred_map->buckets[predicate_bucket]; predicate_entry != NULL && !found; predicate_entry = predicate_entry->next) {
                    if (predicate_id != NO_FILTER && predicate_entry->pred_id != predicate_id) continue;
                    IntSet *object_set = &predicate_entry->objects;
                    if (object_id != NO_FILTER) {
                        if (intset_contains(object_set, object_id)) found = 1;
                    } else {
                        if (object_set->len > 0) found = 1;
                    }
                }
            }
            if (found) {
                const char *subject_str = strings->items[subject_entry->subj_id];
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
        }
        Py_RETURN_NONE;
    }

    typedef struct { size_t subject_id, predicate_id, object_id; } TripleIds;
    TripleIds *to_remove = NULL;
    size_t n_remove = 0;
    size_t cap_remove = 0;

    SPOIndex *spo = &self->spo;
    for (size_t bucket_idx = 0; bucket_idx < spo->n_buckets; bucket_idx++) {
        for (SubjEntry *subject_entry = spo->buckets[bucket_idx]; subject_entry != NULL; subject_entry = subject_entry->next) {
            if (subject_id != NO_FILTER && subject_entry->subj_id != subject_id) continue;
            PredMap *pred_map = &subject_entry->predicates;
            for (size_t predicate_bucket = 0; predicate_bucket < pred_map->n_buckets; predicate_bucket++) {
                for (PredEntry *predicate_entry = pred_map->buckets[predicate_bucket]; predicate_entry != NULL; predicate_entry = predicate_entry->next) {
                    if (predicate_id != NO_FILTER && predicate_entry->pred_id != predicate_id) continue;
                    IntSet *object_set = &predicate_entry->objects;
                    for (size_t i = 0; i < object_set->n_slots; i++) {
                        if (!object_set->occupied[i]) continue;
                        if (object_id != NO_FILTER && object_set->slots[i] != object_id) continue;
                        if (n_remove >= cap_remove) {
                            size_t new_cap = cap_remove == 0 ? 16 : cap_remove * 2;
                            TripleIds *new_buf = realloc(to_remove, new_cap * sizeof(TripleIds));
                            if (new_buf == NULL) {
                                free(to_remove);
                                return PyErr_NoMemory();
                            }
                            to_remove = new_buf;
                            cap_remove = new_cap;
                        }
                        to_remove[n_remove++] = (TripleIds){.subject_id = subject_entry->subj_id, .predicate_id = predicate_entry->pred_id, .object_id = object_set->slots[i]};
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < n_remove; i++) {
        if (spo_remove(&self->spo, to_remove[i].subject_id, to_remove[i].predicate_id, to_remove[i].object_id) == 1) {
            self->len--;
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

    RDFTerm term = {type, value, datatype, lang};
    if (!rdfterm_interner_get(&self->terms, &term, &object_id)) {
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
    {"add", (PyCFunction)TripleLite_add, METH_VARARGS, "Add a triple."},
    {"remove", (PyCFunction)TripleLite_remove, METH_VARARGS, "Remove triples matching pattern."},
    {"triples", (PyCFunction)TripleLite_triples, METH_VARARGS, "Query triples by pattern."},
    {"objects", (PyCFunction)TripleLite_objects, METH_VARARGS | METH_KEYWORDS, "Get objects."},
    {"predicate_objects", (PyCFunction)TripleLite_predicate_objects, METH_VARARGS | METH_KEYWORDS, "Get predicate-object pairs."},
    {"subjects", (PyCFunction)TripleLite_subjects, METH_VARARGS | METH_KEYWORDS, "Get subjects."},
    {"has_subject", (PyCFunction)TripleLite_has_subject, METH_VARARGS, "Check if subject exists."},
    {NULL, NULL, 0, NULL}
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
    .tp_iter = (getiterfunc)TripleLite_iter,
    .tp_as_sequence = &TripleLite_as_sequence,
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

#include <Python.h>
#include "dynarray.h"
#include "hashmap.h"
#include "rdfterm_hashmap.h"

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

static int string_interner_get_or_create(StringInterner *interner, const char *s, size_t *out) {
    if (hashmap_get(&interner->str_to_id, s, out) == 1) {
        return 0;
    }
    size_t new_id = interner->id_to_str.len;
    if (string_array_append(&interner->id_to_str, s) == -1 || hashmap_put(&interner->str_to_id, s, new_id) == -1) {
        return -1;
    }
    *out = new_id;
    return 0;
}

static int string_interner_get(StringInterner *interner, const char *s, size_t *out)
{
    return hashmap_get(&interner->str_to_id, s, out);
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

static PyObject *
hello(PyObject *self, PyObject *args)
{
    return PyUnicode_FromString("Hello from C extension!");
}

static PyMethodDef core_methods[] = {
    {"hello", hello, METH_NOARGS, "Return a greeting from the C extension."},
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
    return PyModule_Create(&core_module);
}
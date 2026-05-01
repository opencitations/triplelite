#include <Python.h>

typedef struct {
    const char *type;
    const char *value;
    const char *datatype;
    const char *lang;
} RDFTerm;

typedef struct {
    const char **items;
    size_t len;
    size_t capacity;
} StringArray;

typedef struct {
    RDFTerm *items;
    size_t len;
    size_t capacity;
} RDFTermArray;

static void string_array_init(StringArray *arr)
{
    arr->items = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

static int string_array_append(StringArray *arr, const char *s){
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

static void rdfterm_array_init(RDFTermArray *arr)
{
    arr->items = NULL;
    arr->len = 0;
    arr->capacity = 0;
}

static int rdfterm_array_append(RDFTermArray *arr, const RDFTerm *term){
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
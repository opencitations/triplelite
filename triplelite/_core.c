#include <Python.h>
#include "dynarray.h"
#include "hashmap.h"

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
#include <Python.h>
#include <stdio.h>
#include <assert.h>

#include <caml/alloc.h>
#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/callback.h>

#define PYERR_IF(boolean, body) if(boolean){body}else{PyErr_Print();}
#define ANSI_GREEN  "\x1b[32m"
#define ANSI_RESET  "\x1b[0m"
#define ANSI_RED    "\x1b[31m"
#define MODULE "pyml"
#define PYTHON "call"

static PyObject *call(PyObject *self, PyObject *args) {
    char *f;
    int arg;

    if(!PyArg_ParseTuple(args, "si:call", &f, &arg)) {
        PyErr_SetString(PyExc_Exception, "tuple not complete.");
        return NULL;
    }

    value *func = caml_named_value(f);
    if (func == NULL) {
        printf(ANSI_RED "\tFAIL\t" ANSI_RESET "Error retrieving function.\n");
        PyErr_SetString(PyExc_Exception, "function does not exist.");

        return NULL;
    } else {
        PyObject *result = Py_BuildValue("i", Int_val(caml_callback(*func, Val_int(arg))));

        return result;
    }
}

static PyMethodDef pyml_methods[] = {
    {"call", call, METH_VARARGS,
     "Calls an OCaml function."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpyml(void) {
    (void) Py_InitModule(MODULE, pyml_methods);
}

CAMLprim value call_python(value f, value arg) {
    PyObject *name, *module, *func, *args, *val;
    name = PyString_FromString(PYTHON);
    module = PyImport_Import(name);

    PYERR_IF(module != NULL,
        args = PyTuple_New(1);
        val = PyLong_FromLong(Long_val(arg));
        PyTuple_SetItem(args, 0, val);
        func = PyObject_GetAttrString(module, String_val(f));
        PYERR_IF(PyCallable_Check(func),
            PyObject *x = PyObject_CallObject(func, args);
            Py_DECREF(val);
            Py_DECREF(args);

            return Val_int((int) PyLong_AsLong(x));
        );
    );

    return Val_unit;
}

static void run_python_test(char *f, int expected) {
    printf("[PY] Running %s... ", f);
    PyObject *name, *module, *func;
    name = PyString_FromString(PYTHON);
    module = PyImport_Import(name);

    PYERR_IF(module != NULL,
        func = PyObject_GetAttrString(module, f);
        PYERR_IF(PyCallable_Check(func),
            PyObject *x = PyObject_CallObject(func, NULL);
            assert((int)PyLong_AsLong(x) == expected);
            Py_DECREF(x);

            printf(ANSI_GREEN "\tOK\n" ANSI_RESET);
        );
    );
}

static void run_ocaml_test(char *f, int expected) {
    CAMLparam0();
    CAMLlocal1(res);

    printf("[ML] Running %s... ", f);
    value *func = caml_named_value(f);
    if (func == NULL) {
        printf(ANSI_RED "\tFAIL\t" ANSI_RESET "Error retrieving function.\n");
    } else {
        res = caml_callback(*func, Val_unit);
        assert(Int_val(res) == expected);

        printf(ANSI_GREEN "\tOK\n" ANSI_RESET);
    }

    CAMLreturn0;
}

static void run_tests() {
    run_python_test("test_call", 10);
    run_ocaml_test("test_call", 10);
}

int main(int argc, char **argv) {
    Py_Initialize(); // initialize python interpreter
    PySys_SetPath("");
    initpyml();
    //Py_InitModule(MODULE, pyml_methods);
    caml_startup(argv);

    run_tests();

    return 0;
}

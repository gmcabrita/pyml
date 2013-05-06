#include <Python.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <caml/alloc.h>
#include <caml/mlvalues.h>
#include <caml/memory.h>
#include <caml/callback.h>
#include <caml/fail.h>

#define PYERR_IF(boolean, body) if(boolean){body}else{PyErr_Print();}
#define ANSI_GREEN  "\x1b[32m"
#define ANSI_RESET  "\x1b[0m"
#define ANSI_RED    "\x1b[31m"
#define MODULE "pyml"
#define PYTHON "call"

static void raise_python(char *exception) {
    PyErr_SetString(PyExc_Exception, exception);
}

static void raise_ocaml(char *exception) {
    caml_raise_with_string(*caml_named_value("pyml_exception"), exception);
}

static bool check_py() {
    PyObject *ptype, *pvalue, *ptraceback;
    // fetch python exception information
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyErr_Clear();

    if (ptype != NULL) {
        raise_ocaml(PyString_AsString(PyObject_Str(pvalue)));

        Py_XDECREF(ptype);
        Py_XDECREF(pvalue);
        Py_XDECREF(ptraceback);

        return true;
    }

    Py_XDECREF(ptype);
    Py_XDECREF(pvalue);
    Py_XDECREF(ptraceback);

    return false;
}

static bool check_ml(value res) {
    CAMLlocal1(exc);

    if (Is_exception_result(res)) {
        exc = Extract_exception(res);
        exc = caml_callback(*caml_named_value("Printexc.to_string"), exc);
        raise_python(String_val(exc));

        return true;
    }

    return false;
}

static PyObject *call(PyObject *self, PyObject *args) {
    char *f;
    int arg;
    CAMLlocal1(res);

    // unpack the python tuple
    if(!PyArg_ParseTuple(args, "si:call", &f, &arg)) {
        PyErr_SetString(PyExc_Exception, "tuple not complete.");
        return NULL;
    }

    value *func = caml_named_value(f);
    if (func == NULL) {
        PyErr_SetString(PyExc_Exception, "function does not exist.");

        return NULL;
    } else {
        // call ocaml function and check if an exception was raised
        res = caml_callback_exn(*func, Val_int(arg));
        if (check_ml(res)) return NULL;

        PyObject *result = Py_BuildValue("i", Int_val(res));

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
    // load module
    name = PyString_FromString(PYTHON);
    module = PyImport_Import(name);

    PYERR_IF(module != NULL,
        // build python tuple to use as argument
        args = PyTuple_New(1);
        val = PyLong_FromLong(Long_val(arg));
        PyTuple_SetItem(args, 0, val);

        // call python function
        func = PyObject_GetAttrString(module, String_val(f));
        PYERR_IF(PyCallable_Check(func),
            PyObject *x = PyObject_CallObject(func, args);

            // check if the python interpreter raised any exceptions
            if (check_py()) return Val_unit;

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
    // run simple call tests
    run_python_test("test_call", 10);
    run_ocaml_test("test_call", 10);

    // run exception raise/catch test
    run_python_test("test_exception", 1);
    run_python_test("test_custom_exception", 1);
    run_ocaml_test("test_exception", 1);
    run_ocaml_test("test_custom_exception", 1);
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

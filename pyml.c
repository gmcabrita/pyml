#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
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

value py_to_ml(PyObject *obj) {
    if (PyBool_Check(obj) == true) {
        if (PyObject_IsTrue(obj)) {
            return Val_bool(true);
        } else {
            return Val_bool(false);
        }
    } else if (PyInt_Check(obj) == true) {
        return Val_int(PyInt_AsLong(obj));
    } else if (PyLong_Check(obj) == true) {
        return Val_long(PyInt_AsLong(obj));
    } else if (PyString_Check(obj) == true) {
        return caml_copy_string(PyString_AsString(obj));
    } else if (PyFloat_Check(obj) == true) {
        return caml_copy_double(PyFloat_AsDouble(obj));
    } else {
        return Val_unit;
    }
}

PyObject *ml_to_py(value val) {
    if (Is_long(val)) {
        return Py_BuildValue("i", Int_val(val));
    } else if (val == Val_true || val == Val_false) {
        return PyBool_FromLong(Bool_val(val));
    } else {
        switch(Tag_val(val)) {
            case String_tag:
                return Py_BuildValue("s", String_val(val));
                break;
            case Double_tag:
                return Py_BuildValue("d", Double_val(val));
                break;
            default:
                return NULL;
                break;
        }
    }
}

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

static value *unpack_python(PyObject *args) {
    int list_size = (int)PyList_Size(args);
    PyObject *tmp;
    value *arg_array = malloc(sizeof(value) * list_size);

    for(int i=0; i < list_size; i++) {
        tmp = (PyObject*)PyList_GetItem(args, (Py_ssize_t)i);
        arg_array[i] = py_to_ml(tmp);
    }

    Py_XDECREF(tmp);
    return arg_array;
}

static PyObject *call_ocaml(PyObject *self, PyObject *args) {
    char *f;
    PyListObject *arg;
    CAMLlocal1(res);

    // unpack the python tuple
    if(!PyArg_ParseTuple(args, "sO!:call", &f, &PyList_Type, &arg)) {
        PyErr_SetString(PyExc_Exception, "tuple not complete.");
        return NULL;
    }

    // unpack the list of python arguments
    value *unpacked_args = unpack_python((PyObject*)arg);
    int argc = (int)PyList_Size((PyObject*)arg);
    Py_XDECREF(arg);

    value *func = caml_named_value(f);
    if (func == NULL) {
        PyErr_SetString(PyExc_Exception, "function does not exist.");

        return NULL;
    } else {
        // call ocaml function and check if an exception was raised
        res = caml_callbackN_exn(*func, argc, unpacked_args);
        if (check_ml(res)) return NULL;

        return ml_to_py(res);
    }
}

static PyMethodDef pyml_methods[] = {
    {"call", call_ocaml, METH_VARARGS,
     "Calls an OCaml function."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initpyml(void) {
    (void) Py_InitModule(MODULE, pyml_methods);
}

static PyObject *unpack_ocaml(value list) {
    CAMLparam1(list);
    CAMLlocal2(head, tmp);
    head = list;
    PyObject *args;
    PyObject *val;

    int i;
    for(i = 0; list != Val_emptylist; i++) {
        list = Field(list, 1);
    }

    list = head;
    args = PyTuple_New(i);
    for(int j=0; list != Val_emptylist; j++) {
        tmp = Field(list, 0);
        val = ml_to_py(tmp);

        PyTuple_SetItem(args, j, val);
        list = Field(list, 1);
    }

    return args;
}

CAMLprim value call_python(value f, value argv) {
    CAMLparam1(argv);
    PyObject *name, *module, *func, *args;

    // load module
    name = PyString_FromString(PYTHON);
    module = PyImport_Import(name);

    if (module != NULL) {
        // build python tuple to use as argument
        args = unpack_ocaml(argv);

        // call python function
        func = PyObject_GetAttrString(module, String_val(f));
        if (PyCallable_Check(func)) {
            PyObject *x = PyObject_CallObject(func, args);

            Py_XDECREF(args);
            Py_XDECREF(func);
            Py_XDECREF(module);
            Py_XDECREF(name);

            // check if the python interpreter raised any exceptions
            if (check_py()) return Val_unit;


            return py_to_ml(x);
        }
    }

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

            if (PyBool_Check(x) == true) {
                if (PyObject_IsTrue(x)) {
                    assert(expected);
                } else {
                    assert(!expected);
                }
            } else if (PyInt_Check(x) == true) {
                assert((int)PyInt_AsLong(x) == expected);
            } else if (PyLong_Check(x) == true) {
                assert(PyInt_AsLong(x) == (long)expected);
            }

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

        if (Is_long(res)) {
            assert(Int_val(res) == expected);
        } else if (res == Val_true) {
            assert(expected);
        } else if (res == Val_false) {
            assert(!expected);
        }

        printf(ANSI_GREEN "\tOK\n" ANSI_RESET);
    }

    CAMLreturn0;
}

static void run_tests() {
    run_python_test("test_call", 10);
    run_python_test("test_exception", 1);
    run_python_test("test_custom_exception", 1);
    run_python_test("test_n_args", 5);
    run_python_test("test_call_with_double", 4);
    run_python_test("test_call_with_string", 0);
    run_python_test("test_call_with_bool", (int)false);

    run_ocaml_test("test_call", 10);
    run_ocaml_test("test_exception", 1);
    run_ocaml_test("test_custom_exception", 1);
    run_ocaml_test("test_n_args", 5);
    run_ocaml_test("test_call_with_double", 4);
    run_ocaml_test("test_call_with_string", (int)true);
    run_ocaml_test("test_call_with_bool", (int)false);
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

import sys
import pyml

sys.path = ['', '/usr/lib/python2.7', '/usr/lib/python2.7/plat-linux2', '/usr/lib/python2.7/lib-tk', '/usr/lib/python2.7/lib-old', '/usr/lib/python2.7/lib-dynload', '/usr/local/lib/python2.7/dist-packages', '/usr/lib/python2.7/dist-packages']


def pass_integer(n):
    return n


def test_raise(n):
    raise Exception("pyml_test_exception")


def test_custom_exception():
    try:
        pyml.call("test_raise", [0])
    except:
        return 1


def test_exception():
    try:
        pyml.call("divide_by", [0])
    except:
        return 1


def divide_by(n):
    return 5 / n


def divide(x, y):
    return x / y


def test_n_args():
    return pyml.call("divide", [10, 2])


def test_call():
    return pyml.call("pass_integer", [10])


def test_call_with_double():
    return int(pyml.call("fdivide", [5.5, 1.2]))


def test_call_with_string():
    return pyml.call("strcmp", ["UROP", "UROP"])


def test_call_with_bool():
    return pyml.call("boolcmp", [True, False])


def strcmp(x, y):
    return int(x == y)


def boolcmp(x, y):
    return x == y

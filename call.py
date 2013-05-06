import sys
import pyml

sys.path = ['', '/usr/lib/python2.7', '/usr/lib/python2.7/plat-linux2', '/usr/lib/python2.7/lib-tk', '/usr/lib/python2.7/lib-old', '/usr/lib/python2.7/lib-dynload', '/usr/local/lib/python2.7/dist-packages', '/usr/lib/python2.7/dist-packages']


def pass_integer(n):
    return n


def test_raise(n):
    raise Exception("pyml_test_exception")


def test_custom_exception():
    try:
        pyml.call("test_raise", 0)
    except:
        return 1


def test_exception():
    try:
        pyml.call("divide_by", 0)
    except:
        return 1


def divide_by(n):
    return 5 / n


def test_call():
    return pyml.call("pass_integer", 10)

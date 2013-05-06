OCAMLC?=ocamlc
PYTHON?=python2.7
CC=gcc -fPIC -std=c99

default:
	mkdir -p bin
	$(OCAMLC) -output-obj call.ml -o call.o
	$(CC) -c pyml.c -o pyml.o -I/usr/include/$(PYTHON)/ -I"`ocamlc -where`"
	$(CC) -o bin/pyml call.o pyml.o -L"`ocamlc -where`" -lcamlrun -I/usr/include/$(PYTHON)/ -l$(PYTHON) -lcurses
	$(CC) pyml.o call.o -shared -o pyml.so -I/usr/include/$(PYTHON)/ -l$(PYTHON) -I"`ocamlc -where`" -L"`ocamlc -where`" -lcamlrun -lcurses

run: default
	./bin/pyml

clean:
	rm -rf bin/
	rm -rf build/
	rm -f *.cmi
	rm -f *.cmx
	rm -f *.cmo
	rm -f *.mli
	rm -f *.cma
	rm -f *.so
	rm -f *.o
	rm -f *.a
	rm -f *.pyc

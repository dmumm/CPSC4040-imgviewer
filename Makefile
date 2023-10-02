OFILES = \
	base/Image.o \
	base/BasicViewer.o \
	base/imgviewer.o

ROOTDIR = .
LIBVIEWER = $(ROOTDIR)/lib/libviewer.a
LIBIMAGE = $(ROOTDIR)/lib/libimage.a

GLLDFLAGS = -lglut -lGL -lm -lGLU

CXX = g++ -Wall -g -O0 -fPIC $(DEFINES) -fopenmp -std=c++14

INCLUDES = -I../build/include/ -I./include/ -I/usr/include


.C.o:
	$(CXX) -c $(INCLUDES) $< -o $@

base: $(OFILES)
	ar rv $(LIBVIEWER) $?
	ar rv $(LIBIMAGE) $?
	$(CXX) base/imgviewer.C $(INCLUDES) -L./lib -lviewer -limage -L../build/lib -lOpenImageIO_Util -lOpenImageIO $(GLLDFLAGS) -o bin/imgviewer

env:
	export LD_LIBRARY_PATH=../build/lib

clean:
	rm -rf bin/imgviewer doc/html *.o base/*.o base/*~ include/*~ python/*~ $(LIBVIEWER) $(LIBIMAGE) *~ swig/*.cxx swig/*~ swig/*.so swig/*.o swig/StarterViewer.py swig/*.pyc ./*.pyc python/*StarterViewer*

all: clean env base

.PHONY: doc
doc:
	cd doc;doxygen Doxyfile
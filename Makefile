# OFILES = \
# 	base/Image.o \
# 	base/BasicViewer.o \
# 	base/ImageProcessor.o \
# 	base/Stencil.o \
# 	base/imgviewer.o

# ROOTDIR = .
# LIBVIEWER = $(ROOTDIR)/lib/libviewer.a
# LIBIMAGE = $(ROOTDIR)/lib/libimage.a
# LIBSTENCIL = $(ROOTDIR)/lib/libstencil.a
# LIBPROCESSOR = $(ROOTDIR)/lib/libprocessor.a

# ANSI color codes
RED   = \033[1;31m
GREEN = \033[1;32m
BLUE  = \033[1;34m
CYAN   = \033[1;36m
RESET = \033[0m

SRCS = $(wildcard base/*.C)
OFILES = $(SRCS:.C=.o)
LIBS = $(OFILES:base/%.o=lib/lib%.a)

MAIN = base/imgviewer.C
OFILES := $(filter-out $(MAIN:.C=.o), $(OFILES))


GLLDFLAGS = -lglut -lGL -lm -lGLU

CXX = g++ -Wall -g -O3 -fPIC $(DEFINES) -fopenmp -std=c++14

INCLUDES = -I../build/include/ -I./include/ -I/usr/include


.C.o:
	@echo "$(BLUE)Compiling $<...$(RESET)"
	$(CXX) -c $(INCLUDES) $< -o $@

# base: $(OFILES)
# 	ar r $(LIBVIEWER) $?
# 	ar r $(LIBIMAGE) $?
# 	ar r $(LIBPROCESSOR) $?
# 	ar r $(LIBSTENCIL) $?
# 	$(CXX) base/imgviewer.C $(INCLUDES) -L./lib -lviewer -limage -lprocessor -lstencil -L../build/lib -lOpenImageIO_Util -lOpenImageIO $(GLLDFLAGS) -o bin/imgviewer

lib/lib%.a: base/%.o | create_directories
	@echo "$(GREEN)Archiving $@...$(RESET)"
	ar r $@ $<

bin/imgviewer: $(OFILES) $(LIBS) | create_directories
	@echo "$(CYAN)Linking $@...$(RESET)"
	$(CXX) base/imgviewer.C $(INCLUDES) -L./lib -Wl,--start-group $(LIBS) -Wl,--end-group -L../build/lib -lOpenImageIO_Util -lOpenImageIO $(GLLDFLAGS) -o $@

all: clean create_directories base/imgviewer
	@echo "$(GREEN)Build complete.$(RESET)"

clean:
	@echo "$(RED)Cleaning up...$(RESET)"
	rm -rf bin/* lib/*.a doc/html *.o base/*.o base/*~ include/*~ python/*~ *~ swig/*.cxx swig/*~ swig/*.so swig/*.o swig/StarterViewer.py swig/*.pyc ./*.pyc python/*StarterViewer*

.PHONY: create_directories

create_directories:
	@mkdir -p bin doc lib
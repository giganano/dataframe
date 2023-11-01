
CC 					:= gcc
CFLAGS				:= -c -fPIC -Wsign-conversion -Wsign-compare
SOURCES				:= $(wildcard *.src.c)
CYTHON_SOURCES 		:= $(wildcard *.pyx)
OUTPUTS 			:= $(SOURCES:.src.c=.src.o)
CYTHON_OUTPUTS 		:= $(CYTHON_SOURCES:.pyx=.c)
HEADERS 			:= $(SOURCES:.src.c=.src.h)
INCLUDE 			:=

all: $(OUTPUTS)

%.src.o: %.src.c $(HEADERS)
ifdef INCLUDE
	$(CC) $(CFLAGS) -I$(INCLUDE) $< -o $@
else
	$(CC) $(CFLAGS) $< -o $@
endif

.PHONY: clean
clean:
	@ rm -rf __pycache__
	@ for i in $(OUTPUTS) ; do \
		rm -f $$i ; \
	done
	@ for i in $(CYTHON_OUTPUTS) ; do \
		rm -f $$i ; \
	done
	@ rm -f *.so


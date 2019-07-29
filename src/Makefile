# Variables for file compilation
CC        =  gcc
CFLAGS    = -O3 -march=native -w -I../include
CPPFLAGS  =
LDFLAGS   = -g -lpapi -lpthread -lhwloc
LDLIBS    =

TARGETS = clwb_all.run

CLWB_DEPS = clwb_all_impl.c

CLWB_DEPS_OBJ= $(patsubst %.c, %.o, $(CLWB_DEPS))

DEPS = $(wildcard *.h)

all: $(TARGETS)

clwb_all.run: clwb_all.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c $< $(CFLAGS)

clean:
	rm -rf *.o *.run *~

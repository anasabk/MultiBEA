OBJECTS = obj/common.o \
		  obj/stdgraph.o \
		  obj/genetic_alg.o \
		  obj/test.o

SEARCH_PATH = include

CFLAGS = -Wall -std=gnu17 -I$(SEARCH_PATH)
LINKFLAGS = -lm

ifdef WITH_DEBUG
  CFLAGS += -g -D WITH_DEBUG=1
else
  CFLAGS += -O3
endif

obj/%.o: src/%.c
	mkdir -p $(@D)
	gcc $(CFLAGS) -c $< -o $@ $(LINKFLAGS)

build: $(OBJECTS)
	gcc $(CFLAGS) $(OBJECTS) -o genetic_coloring $(LINKFLAGS)

run:
	./genetic_coloring ../test_list.txt 3 3 100000

clean:
	rm -rf obj genetic_coloring

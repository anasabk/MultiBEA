OBJECTS = obj/common.o \
		  obj/stdgraph.o \
		  obj/genetic_alg.o \
		  obj/test.o 

SEARCH_PATH = include

CFLAGS = -Wall -std=gnu17 -I$(SEARCH_PATH)
LINKFLAGS = -lm

ifdef DEBUG
  CFLAGS += -g -D WITH_DEBUG=1
else
  CFLAGS += -O3
endif

obj/%.o: src/%.c
	mkdir -p $(@D)
	gcc $(CFLAGS) -c $< -o $@ $(LINKFLAGS)

build: $(OBJECTS)
	gcc $(CFLAGS) $(OBJECTS) -o MultiBEA $(LINKFLAGS)

run: # ./graph_coloring test_list #_of_parallel_tests
	./MultiBEA tests/iteration_tests.txt results/DIMACS_test.txt 1

clean:
	rm -rf obj MultiBEA

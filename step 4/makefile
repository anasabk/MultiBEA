OBJECTS = obj/common.o \
		  obj/stdgraph.o \
		  obj/genetic_alg.o \
		  obj/test.o

SEARCH_PATH = include

CFLAGS = -Wall -std=gnu17 -lm -I$(SEARCH_PATH)

ifdef WITH_DEBUG
  CFLAGS += -g -D WITH_DEBUG=1
else
  CFLAGS += -O3
endif

obj/%.o: src/%.c
	mkdir -p $(@D)
	gcc $(CFLAGS) -c $< -o $@

build: $(OBJECTS)
	gcc $(CFLAGS) $(OBJECTS) -o genetic_coloring

run: build
	./genetic_coloring resources/test_list.txt

clean:
	rm -rf obj genetic_coloring
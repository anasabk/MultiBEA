OBJECTS = obj/crossover.o \
		  obj/genetic_alg.o \
		  obj/test.o

SEARCH_PATH = include

GCCPARAMS = -Wall -g -std=gnu17 -lm -I$(SEARCH_PATH)

obj/%.o: src/%.c
	mkdir -p $(@D)
	gcc $(GCCPARAMS) -c $< -o $@

build: $(OBJECTS)
	gcc $(GCCPARAMS) $(OBJECTS) -o genetic_coloring

run: build
	./genetic_coloring resources/test_list.txt

clean:
	rm -rf obj genetic_coloring
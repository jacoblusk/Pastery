CC ?= clang

EXECUTABLE = pastery
INCLUDE = -I/usr/local/include -Iinclude -I$$HOME/.local/include $(shell python3.5-config --cflags)

all: $(EXECUTABLE)

$(EXECUTABLE): obj/main.o obj/strbuilder.o obj/utility.o obj/generator.o obj/memstream.o obj/pygments.o
	$(CC) $^ -o $@ $$HOME/.local/lib/libctemplate.a -lfcgi -lhiredis `python3.5-config --ldflags` -L/usr/local/lib -L$$HOME/.local/lib 


obj/%.o: src/%.c
	$(CC) $< -Wall -Wextra -pedantic -g -c -o $@ $(INCLUDE)

obj/main.o: src/main.c include/pygments.h include/utility.h include/error.h
obj/strbuilder.o: src/strbuilder.c include/strbuilder.h include/error.h
obj/utility.o: src/utility.c include/utility.h include/strbuilder.h
obj/generator.o: src/generator.c include/error.h
obj/memstream.o: src/memstream.c include/memstream.h
obj/pygments.o: src/pygments.c include/pygments.h

clean:
	$(RM) $(EXECUTABLE) obj/*

.PHONY: clean

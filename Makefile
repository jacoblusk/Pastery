CC ?= clang

EXECUTABLE = pastery
INCLUDE = -I/usr/local/include -Iinclude

all: $(EXECUTABLE)

$(EXECUTABLE): obj/main.o obj/strbuilder.o obj/utility.o
	$(CC) $^ -o $@ -lfcgi -lctemplate -L/usr/local/lib

obj/main.o: src/main.c
	$(CC) src/main.c -c -o $@ $(INCLUDE)

obj/strbuilder.o: src/strbuilder.c include/strbuilder.h include/error.h
	$(CC) src/strbuilder.c -c -o $@ $(INCLUDE)

obj/utility.o: src/utility.c include/utility.h include/strbuilder.h
	$(CC) src/utility.c -c -o $@ $(INCLUDE)

clean:
	$(RM) $(EXECUTABLE) obj/*

.PHONY: clean

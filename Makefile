CC = gcc
CFLAGS = -O3 -Wall -Wextra -Winline --std=c99
CLIBS = -lsoundio -lkissfft-float -lsdl2
OBJECTS = $(addprefix bin/,synth.o operator.o window.o player.o envelope.o note.o song.o export.o filter.o font.o)

font:
	python3 tools/make-font.py assets/font.png > src/font-data.h

bin:
	mkdir -p bin

bin/%.o: src/%.c src/%.h bin font
	$(CC) -o $@ -c $< $(CFLAGS)

objects: $(OBJECTS)

fm: objects bin main.c
	$(CC) main.c -o bin/fm $(OBJECTS) $(CFLAGS) $(CLIBS)

run: fm
	bin/fm

clean:
	rm -r bin

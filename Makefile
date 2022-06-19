CC = gcc
CFLAGS = -Wall -Wextra -Winline --std=c99
CLIBS = -lsoundio -lkissfft-float -lsdl2 -lsdl2_ttf
OBJECTS = $(addprefix bin/,synth.o operator.o window.o player.o envelope.o note.o song.o)

bin:
	mkdir -p bin

bin/%.o: src/%.c src/%.h bin
	$(CC) -o $@ -c $< $(CFLAGS)

objects: $(OBJECTS)

fm: objects bin main.c
	$(CC) main.c -o bin/fm $(OBJECTS) $(CFLAGS) $(CLIBS)

run: fm
	bin/fm

clean:
	rm -r bin

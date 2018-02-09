all: mazegame tr

HEADERS=assert.h blocks.h input.h maze.h modex.h text.h Makefile
OBJS=assert.o mazegame.o maze.o blocks.o modex.o input.o text.o

CFLAGS=-g -Wall

mazegame: ${OBJS}
	gcc -g -o mazegame ${OBJS}

tr: modex.c ${HEADERS} text.o
	gcc ${CFLAGS} -DTEXT_RESTORE_PROGRAM=1 -o tr modex.c text.o

%.o: %.c ${HEADERS}
	gcc ${CFLAGS} -c -o $@ $<

%.o: %.s ${HEADERS}
	gcc ${CFLAGS} -c -o $@ $<

clean::
	rm -f *.o *~ a.out

clear: clean
	rm -f mazegame tr

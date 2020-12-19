CFLAGS=-std=c11 -g -static
SRC_READ=$(wildcard ./src/read/*.c)
SRC_OBJS=$(SRC_READ:.c=.o)


MAIN_READ=$(wildcard ./src/*.c)
MAIN_OBJS=$(MAIN_READ:.c=.o)

ALL: jcc

$(SRC_OBJS): ./src/read/read.h

$(MAIN_OBJS): ./src/jcc.h

jcc:$(SRC_OBJS) $(MAIN_OBJS)
	$(CC) -o jcc $(SRC_OBJS) $(MAIN_OBJS)

clean:
	rm -f jcc ./src/*.o tmp* ./src/*/*.o

.PHONY: clean
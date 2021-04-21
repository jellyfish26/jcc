CFLAGS=-std=c11 -g -static
SRC_READ=$(wildcard ./src/*/*.c)
SRC_OBJS=$(SRC_READ:.c=.o)


MAIN_READ=$(wildcard ./src/*.c)
MAIN_OBJS=$(MAIN_READ:.c=.o)

ALL: jcc

$(SRC_OBJS): ./src/*/*.h

jcc:$(SRC_OBJS) $(MAIN_OBJS)
	$(CC) -o jcc $(SRC_OBJS) $(MAIN_OBJS)

compile: 
	./jcc tmp.c > tmp.s
	$(CC) -static -o tmp tmp.s

test: jcc
	./test.sh

clean:
	rm -f jcc ./src/*.o tmp* ./src/*/*.o

.PHONY: test clean

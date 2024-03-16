CFLAGS=-Wall -Wextra -pedantic -std=c99 -O2 -g -Wmissing-prototypes
TARGET=fifo

all default: ${TARGET}

run test tests: ${TARGET}
	${TRACE} ./${TARGET}

${TARGET}: main.c fifo.h
	${CC} ${CFLAGS} $< -o $@

lib${TARGET}.a: lib.c fifo.h
	${AR} rcs $@ $<

clean:
	git clean -dffx

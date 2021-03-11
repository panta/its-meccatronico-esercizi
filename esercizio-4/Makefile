# uncomment to enable debug, comment to disable
DEBUG ?= y

BINARIES ?= mandelbrot-single mandelbrot-multi

CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Wstrict-prototypes -Wconversion -Werror
CC = gcc
CPP = cpp

ifdef DEBUG
CFLAGS += -g
else
CFLAGS += -O2
endif

N_THREADS ?= 16

all: depend $(BINARIES)

compute-single: mandelbrot-single
	./mandelbrot-single >/dev/null
	time ./mandelbrot-single >mandel-single.ppm

compute-multi: mandelbrot-multi
	./mandelbrot-multi $(N_THREADS) >/dev/null
	time ./mandelbrot-multi $(N_THREADS) >mandel-multi.ppm

-include .depend

clean:
	-rm -f .depend *.o $(BINARIES)
	@-rm -rf *.dSYM

depend :
	@rm -f .depend
	@for i in *.c; do  \
		$(CC) -MM $(CFLAGS) $$i >> .depend ; \
	done

# automatic rule to compile .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# automatic rule to compile .c files directly into executable files
%: %.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lpthread

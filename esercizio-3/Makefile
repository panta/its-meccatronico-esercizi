# uncomment to enable debug, comment to disable
DEBUG ?= y

BINARIES ?= vec-random dot-single dot-multi

CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Wstrict-prototypes -Wconversion -Werror
CC = gcc
CPP = cpp

ifdef DEBUG
CFLAGS += -g
else
CFLAGS += -O2
endif

VEC_SIZE ?= 1000000
N_THREADS ?= 4

all: depend $(BINARIES)

u: vec-random
	./vec-random $(VEC_SIZE) >$@
v: vec-random
	./vec-random $(VEC_SIZE) >$@

compute-single: u v dot-single
	cat u v | ./dot-single $(VEC_SIZE) >/dev/null
	time cat u v | ./dot-single $(VEC_SIZE)

compute-multi: u v dot-multi
	cat u v | ./dot-multi $(VEC_SIZE) $(N_THREADS) >/dev/null
	time cat u v | ./dot-multi $(VEC_SIZE) $(N_THREADS)

-include .depend

clean:
	-rm -f .depend *.o $(BINARIES) u v
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

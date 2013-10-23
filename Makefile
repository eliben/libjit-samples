#-------------------------------------------------------------------------------
# Makefile for building the code samples. Read inline comments for
# documentation.
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------

LIBJIT_PATH = $$HOME/test/libjit
LIBJIT_INCLUDE_PATH = $(LIBJIT_PATH)/include
LIBJIT_LIB_PATH = $(LIBJIT_PATH)/jit/.libs
LIBJIT_AR = $(LIBJIT_LIB_PATH)/libjit.a

CC = gcc
LD = gcc
CCOPT = -g -O0
CCFLAGS = -c $(CCOPT)
LDFLAGS = -lpthread -lm -ldl -lrt

all: gcd gcd_iter call_c_from_jit call_puts_from_jit

gcd: gcd.o
	$(LD) $^ $(LIBJIT_AR) $(LDFLAGS) -o $@

gcd.o: gcd.c
	$(CC) -I$(LIBJIT_INCLUDE_PATH) -I. $(CCFLAGS) $^ -o $@

gcd_iter: gcd_iter.o
	$(LD) $^ $(LIBJIT_AR) $(LDFLAGS) -o $@

gcd_iter.o: gcd_iter.c
	$(CC) -I$(LIBJIT_INCLUDE_PATH) -I. $(CCFLAGS) $^ -o $@

call_c_from_jit: call_c_from_jit.o
	$(LD) $^ $(LIBJIT_AR) $(LDFLAGS) -o $@

call_c_from_jit.o: call_c_from_jit.c
	$(CC) -I$(LIBJIT_INCLUDE_PATH) -I. $(CCFLAGS) $^ -o $@

call_puts_from_jit: call_puts_from_jit.o
	$(LD) $^ $(LIBJIT_AR) $(LDFLAGS) -o $@

call_puts_from_jit.o: call_puts_from_jit.c
	$(CC) -I$(LIBJIT_INCLUDE_PATH) -I. $(CCFLAGS) $^ -o $@

clean:
	rm -rf *.o gcd gcd_iter call_c_from_jit call_puts_from_jit



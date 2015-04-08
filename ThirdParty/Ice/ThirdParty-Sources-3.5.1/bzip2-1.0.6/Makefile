# ------------------------------------------------------------------
# This file is part of bzip2/libbzip2, a program and library for
# lossless, block-sorting data compression.
#
# bzip2/libbzip2 version 1.0.6 of 6 September 2010
# Copyright (C) 1996-2010 Julian Seward <jseward@bzip.org>
#
# Please read the WARNING, DISCLAIMER and PATENTS sections in the 
# README file.
#
# This program is released under the terms of the license contained
# in the file LICENSE.
# ------------------------------------------------------------------

SHELL=/bin/sh

# To assist in cross-compiling
CC=gcc -fPIC
AR=ar
RANLIB=ranlib
LDFLAGS=

BIGFILES=-D_FILE_OFFSET_BITS=64
CFLAGS=-Wall -Winline -O2 -g $(BIGFILES)

# Where you want it installed when you do 'make install'
PREFIX=/usr/local


OBJS= blocksort.o  \
      huffman.o    \
      crctable.o   \
      randtable.o  \
      compress.o   \
      decompress.o \
      bzlib.o

bzip2: libbz2.a bzip2.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o bzip2 bzip2.o -L. -lbz2

bzip2recover: bzip2recover.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o bzip2recover bzip2recover.o

libbz2.a: $(OBJS)
	rm -f libbz2.a
	$(AR) cq libbz2.a $(OBJS)
	@if ( test -f $(RANLIB) -o -f /usr/bin/ranlib -o \
		-f /bin/ranlib -o -f /usr/ccs/bin/ranlib ) ; then \
		echo $(RANLIB) libbz2.a ; \
		$(RANLIB) libbz2.a ; \
	fi

install: bzip2 bzip2recover
	if ( test ! -d $(PREFIX)/bin ) ; then mkdir -p $(PREFIX)/bin ; fi
	if ( test ! -d $(PREFIX)/lib ) ; then mkdir -p $(PREFIX)/lib ; fi
	if ( test ! -d $(PREFIX)/include ) ; then mkdir -p $(PREFIX)/include ; fi
	cp -f bzip2 $(PREFIX)/bin/bzip2
	cp -f bzip2 $(PREFIX)/bin/bunzip2
	cp -f bzip2 $(PREFIX)/bin/bzcat
	cp -f bzip2recover $(PREFIX)/bin/bzip2recover
	chmod a+x $(PREFIX)/bin/bzip2
	chmod a+x $(PREFIX)/bin/bunzip2
	chmod a+x $(PREFIX)/bin/bzcat
	chmod a+x $(PREFIX)/bin/bzip2recover
	cp -f bzlib.h $(PREFIX)/include
	chmod a+r $(PREFIX)/include/bzlib.h
	cp -f libbz2.a $(PREFIX)/lib
	chmod a+r $(PREFIX)/lib/libbz2.a
	cp -f bzgrep $(PREFIX)/bin/bzgrep
	ln -s -f $(PREFIX)/bin/bzgrep $(PREFIX)/bin/bzegrep
	ln -s -f $(PREFIX)/bin/bzgrep $(PREFIX)/bin/bzfgrep
	chmod a+x $(PREFIX)/bin/bzgrep
	cp -f bzmore $(PREFIX)/bin/bzmore
	ln -s -f $(PREFIX)/bin/bzmore $(PREFIX)/bin/bzless
	chmod a+x $(PREFIX)/bin/bzmore
	cp -f bzdiff $(PREFIX)/bin/bzdiff
	ln -s -f $(PREFIX)/bin/bzdiff $(PREFIX)/bin/bzcmp
	chmod a+x $(PREFIX)/bin/bzdiff

clean: 
	rm -f *.o libbz2.a bzip2 bzip2recover

blocksort.o: blocksort.c
	@cat words0
	$(CC) $(CFLAGS) -c blocksort.c
huffman.o: huffman.c
	$(CC) $(CFLAGS) -c huffman.c
crctable.o: crctable.c
	$(CC) $(CFLAGS) -c crctable.c
randtable.o: randtable.c
	$(CC) $(CFLAGS) -c randtable.c
compress.o: compress.c
	$(CC) $(CFLAGS) -c compress.c
decompress.o: decompress.c
	$(CC) $(CFLAGS) -c decompress.c
bzlib.o: bzlib.c
	$(CC) $(CFLAGS) -c bzlib.c
bzip2.o: bzip2.c
	$(CC) $(CFLAGS) -c bzip2.c
bzip2recover.o: bzip2recover.c
	$(CC) $(CFLAGS) -c bzip2recover.c


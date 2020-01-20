#!/bin/bash

cat >> ~/pa2/Makefile <<'EOF'

TESTDIR = $(HOME)/pa2/ucsd-cs120-wi16-pa2-tester

test:	$(TESTDIR)/test.c $(TESTDIR)/havoc.c $(TESTDIR)/test.h aux.h umix.h mycode2.h sys.h mycode2.o
	$(CC) $(FLAGS) -I. -o test $(TESTDIR)/havoc.c $(TESTDIR)/test.c mycode2.o

EOF

#!/bin/bash
ln -s $HOME/pa2/ucsd-cs120-wi16-pa2-tester/test.c $HOME/pa2/test.c
ln -s $HOME/pa2/ucsd-cs120-wi16-pa2-tester/test.h $HOME/pa2/test.h
ln -s $HOME/pa2/ucsd-cs120-wi16-pa2-tester/havoc.c $HOME/pa2/havoc.c
str="test:\ttest.c test.h havoc.c aux.h umix.h mykernel2.o sys.h\n\t\$(CC) \$(FLAGS) -o test havoc.c test.c mykernel2.o"
echo $str >> $HOME/pa2/Makefile

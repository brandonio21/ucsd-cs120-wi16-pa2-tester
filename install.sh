#!/bin/bash
ln $HOME/pa2/ucsd-cs120-wi16-pa2-tester/test.c $HOME/pa2/test.c
str="test:\ttest.c aux.h umix.h mykernel2.h sys.h\n\t\$(CC) \$(FLAGS) -o test test.c mykernel2.o"
echo $str >> $HOME/pa2/Makefile

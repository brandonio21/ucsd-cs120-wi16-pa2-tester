#!/bin/bash
ln test.c ../test.c
str="test:\ttest.c aux.h umix.h mykernel2.h sys.h\n\t\$(CC) \$(FLAGS) -o test test.c mykernel2.o"
echo $str >> ../Makefile

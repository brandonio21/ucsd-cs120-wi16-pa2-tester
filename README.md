ucsd-cs120-wi16-pa2-tester
==========================
A collection of tests for Pasquale's CSE120 PA2 at UCSD

Installation
------------
1. Ensure that your PA directory is `~/pa2`
2. `cd ~/pa2`
3. `git clone https://github.com/brandonio21/ucsd-cs120-wi16-pa2-tester`
4. `sh ucsd-cs120-wi16-pa-tester/install.sh`
5. `make test`

## Usage
#### Makefile
the install.sh will echo the test target into the Makefile, 
you can change the file name if necessary.

2020-1-19: The mykernel2.c and mykernel2.h has been changed to mycode2.c and mycode2.h

#### test.c
the test.c contains the tests. you can modify it if you need.
If you have logic in HandleTimerIntr, go to test.h and add a definition:

`#define LOGIC_IN_HANDLETIMERINTR 1` 

By default it is open.

Also, for the project since it is running in single CPU, so whoever calls
EndingProc should also be the one thats running in the CPU, which means the
p given to the EndingProc should be the pid thats calling it.

However, this does not apply to the tests, the p given to the
EndingProc may not be the one thats calling it, so you should consider such
situation in your code to utilize the test. But also, consider such situation, 
where one process may kill another process, and you can see it is resonable to
allow such operation in your projcet.

Running
-------
`./test`

If you notice a problem with tests, documentation, or code quality -
feel free to submit a PR! The original maintainers are no longer at
UCSD, but will approve PRs.
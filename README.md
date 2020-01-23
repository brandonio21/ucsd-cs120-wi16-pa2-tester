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

#### test.c
the test.c contains the tests. you can modify it if you need.
If you have logic in HandleTimerIntr, go to test.h and add a definition:

`#define LOGIC_IN_HANDLETIMERINTR 1` 

By default it is open.

In this test program, there is such behavior that the running process 
may kill another process, which is not tested in the autograder.

Running
-------
`./test`

If you notice a problem with tests, documentation, or code quality -
feel free to submit a PR! The original maintainers are no longer at
UCSD, but will approve PRs.
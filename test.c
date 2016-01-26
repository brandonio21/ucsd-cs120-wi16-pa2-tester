/*
 * pa2 tester
 * bmilton
 * 25 jan 2016
 */
#include "mykernel2.h"
#include "aux.h"
#include "sys.h"
#include <time.h>
#include <stdlib.h>



int totalFailCounter = 0;

int test_fifo_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(FIFO);
  InitSched();
  int proc;
  for (proc = 0; proc < numprocs; proc++) {
    StartingProc(proc);
  }

  for (proc = 0; proc < numprocs; proc++) {
    int next = SchedProc();
    if (next != proc) {
      Printf("FIFO ERR: Received process %d but expected %d", next, proc);
      failCounter++;
    }

    EndingProc(proc);
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test_lifo_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(LIFO);
  InitSched();
  int proc;

  for (proc = 0; proc < numprocs; proc++) {
    StartingProc(proc);
  }

  for (proc = numprocs - 1; proc >= 0; proc--) {
    int next = SchedProc();
    if (next != proc) {
      Printf("LIFO ERR: Received process %d but expected %d", next, proc);
      failCounter++;
    }

    EndingProc(proc);
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test_rr_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(ROUNDROBIN);
  InitSched();
  int prevproc, nextproc, iter, i;

  for (prevproc = 0; prevproc < numprocs; prevproc++) {
    StartingProc(prevproc);
  }

  prevproc = SchedProc();
  for (iter = 0; iter < 5; iter++) {
    for (i = 0; i < numprocs; i++) {
      nextproc = SchedProc();
      if ((prevproc + 1) % numprocs != nextproc &&
          ((prevproc - 1) < 0 ? numprocs - 1 : prevproc - 1) != nextproc) {
        Printf("ROUND ROBIN ERR: Encountered process %d\n when expecting %d or %d\n", 
            nextproc, (prevproc+1)%numprocs, 
            ((prevproc - 1) < 0 ? numprocs - 1 : prevproc - 1));
        failCounter++;
      }
      prevproc = nextproc;
    }
  }

  for (prevproc = 0; prevproc < numprocs; prevproc++)
    EndingProc(prevproc);

  totalFailCounter += failCounter;
  return failCounter;
}

int test_proportional_normal(int numprocs) {
  srand(time(NULL));
  int failCounter = 0;
  SetSchedPolicy(PROPORTIONAL);
  InitSched();
  int i, j;


  int proportions[numprocs];
  int counts[numprocs];
  for (i = 0; i < numprocs; i++) {
    StartingProc(i);
    proportions[i] = (rand() % (100 / numprocs)) + 1;
    counts[i] = 0;
  }

  for (i = 0; i < numprocs; i++) {
    if (MyRequestCPUrate(i, proportions[i]) == -1)
      failCounter++;
  }

  for (i = 0; i < 100; i++) {
    counts[SchedProc()]++;
  }

  for (i = 0; i < numprocs; i++) {
    if (counts[i] - proportions[i] < 0) {
      Printf("PROPORTIONAL ERR: %d requested %d%% but received %d\n", i, 
          proportions[i], counts[i]);
      failCounter++;
    }
  }

  for (i = 0; i < numprocs; i++)
    EndingProc(i);

  totalFailCounter += failCounter;
  return failCounter;
}

int test(int (*testerFunction) (int)) {
  int i, failures;
  failures = 0;
  for (i = 1; i < MAXPROCS; i++) {
    failures += testerFunction(i);
  }
  return failures;
}

void Main() {
  Printf("%d fifo failures\n", test(&test_fifo_normal));
  Printf("%d lifo failures\n", test(&test_lifo_normal));
  Printf("%d roundrobin failures\n", test(&test_rr_normal));
  Printf("%d proportional failures\n", test(&test_proportional_normal));
  //lala

  Printf("%d Failures\n", totalFailCounter);
  if (totalFailCounter == 0)
    Printf("ALL TESTS PASSED!");
}

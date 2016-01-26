/*
 * pa2 tester
 * bmilton
 * 25 jan 2016
 * latest update: 26 jan 2016
 */
#include "mykernel2.h"
#include "aux.h"
#include "sys.h"
#include <time.h>
#include <stdlib.h>



int totalFailCounter = 0;

int inSlackRange(int expected, int actual) {
  double slack = expected * 0.10;
  return abs(actual - expected) <= slack;
}

int test_fifo_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(FIFO);
  InitSched();
  int proc;
  for (proc = 1; proc <= numprocs; proc++) {
    StartingProc(proc);
  }

  for (proc = 1; proc <= numprocs; proc++) {
    int next = SchedProc();
    if (next != proc) {
      Printf("FIFO ERR: Received process %d but expected %d", next, proc);
      failCounter++;
    }

    EndingProc(proc);
  }

  /* check if all process are exited */
  if (SchedProc())
      failCounter++;


  totalFailCounter += failCounter;
  return failCounter;
}

int test_lifo_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(LIFO);
  InitSched();
  int proc;

  for (proc = 1; proc <= numprocs; proc++) {
    StartingProc(proc);
  }

  for (proc = numprocs; proc > 0; proc--) {
    int next = SchedProc();
    if (next != proc) {
      Printf("LIFO ERR: Received process %d but expected %d", next, proc);
      failCounter++;
    }

    EndingProc(proc);
  }

  /* check if all process are exited */
  if (SchedProc())
      failCounter++;

  totalFailCounter += failCounter;
  return failCounter;
}

int test_rr_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(ROUNDROBIN);
  InitSched();
  int prevproc, nextproc, iter, i;

  int trueNext1, trueNext2;

  for (prevproc = 1; prevproc <= numprocs; prevproc++) {
    StartingProc(prevproc);
  }

  prevproc = SchedProc();
  for (iter = 0; iter < 5; iter++) {
    for (i = 1; i <= numprocs; i++) {
      nextproc = SchedProc();
      trueNext1 = (prevproc + 1) > numprocs ? 1 : prevproc+1;
      trueNext2 = (prevproc - 1) < 1 ? numprocs : prevproc-1;
      if (trueNext1 != nextproc && trueNext2 != nextproc){
        Printf("ROUND ROBIN ERR: Encountered process %d\n when expecting %d or %d\n", 
            nextproc, trueNext1, trueNext2);
        failCounter++;
      }
      prevproc = nextproc;
    }
  }

  for (prevproc = 1; prevproc <= numprocs; prevproc++)
    EndingProc(prevproc);

  /* check if all process are exited */
  if (SchedProc())
      failCounter++;


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
  for (i = 1; i <= numprocs; i++) {
    StartingProc(i);
    proportions[i] = (rand() % (100 / numprocs)) + 1;
    counts[i] = 0;
  }

  for (i = 1; i <= numprocs; i++) {
    if (MyRequestCPUrate(i, proportions[i]) == -1) {
      failCounter++;
      Printf("PROPORTIONAL ERR: Process requested %d%% CPU and MyRequestCPUrate wrongly returned -1\n",
          proportions[i]);
    }
  }

  for (i = 0; i < 100; i++) {
    counts[SchedProc()]++;
  }

  for (i = 1; i <= numprocs; i++) {
    if (!inSlackRange(proportions[i], counts[i]) && 
        counts[i] < proportions[i]) {
      Printf("PROPORTIONAL ERR: %d requested %d%% but received %d\n", i, 
          proportions[i], counts[i]);
      failCounter++;
    }
  }

  for (i = 1; i <= numprocs; i++)
    EndingProc(i);

  /* check if all process are exited */
  if (SchedProc())
      failCounter++;

  totalFailCounter += failCounter;
  return failCounter;
}

int test_proportional_hog(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(PROPORTIONAL);
  InitSched();
  int i, iter;
  int counts[numprocs];

  for (i = 1; i <= numprocs; i++) {
    StartingProc(i);
    counts[i] = 0;
  }

  if (MyRequestCPUrate(1, 100) == -1) {
    Printf("PROPORTIONAL2 ERR: A single process requested 100%% CPU and MyRequestCPURate returned -1\n");
    failCounter++;
  }

  for (i = 2; i <= numprocs; i++) {
    if (MyRequestCPUrate(i, 20) != -1) {
      Printf("PROPORTIONAL2 ERR: Process %d requested unavailable space and MyRequestCPUrate returned 0\n",
          i);
      failCounter++;
    }
  }

  for (iter = 0; iter < 100; iter++) {
    counts[SchedProc()]++;
  }

  if (counts[1] < (100 - (numprocs - 1))) {
    Printf("PROPORTIONAL2 ERR: Process 1 should have received %d%% CPU time but got %d%%\n",
        (100 - (numprocs - 1)), counts[1]);
    failCounter++;
  }

  for (i = 2; i <= numprocs; i++) {
    if (counts[i] > 1) {
      Printf("PROPORTIONAL2 ERR: Process %d shouldnt have received >1%% CPU time (Received %d%%) " 
          "since process 1 requested 100%%. \n",
          i, counts[i]);
      failCounter++;
    }
    counts[i] = 0;
  }

  EndingProc(1);
  for (iter = 0; iter < 100; iter++) {
    counts[SchedProc()]++;
  }

  for (i = 2; i <= numprocs; i++) {
    if (!inSlackRange(100 / (numprocs-1), counts[i])) {
      Printf("PROPORTIONAL2 ERR: Process %d was expected to receive %d%% CPU (RR after no requests"
        " for CPU were made), but received %d%%\n", i, (100/(numprocs-1)), counts[i]);
      failCounter++;
    }
  }
  totalFailCounter += failCounter;
  return failCounter;
}

int test(int (*testerFunction) (int)) {
  int i, failures;
  failures = 0;
  for (i = MAXPROCS -1; i < MAXPROCS; i++) {
    failures += testerFunction(i);
  }
  return failures;
}

void Main() {
  Printf("%d fifo failures\n", test(&test_fifo_normal));
  Printf("%d lifo failures\n", test(&test_lifo_normal));
  Printf("%d roundrobin failures\n", test(&test_rr_normal));
  Printf("%d proportional failures\n", test(&test_proportional_normal));
  Printf("%d proportional2 failures\n", test(&test_proportional_hog));

  Printf("%d Failures\n", totalFailCounter);
  if (totalFailCounter == 0)
    Printf("ALL TESTS PASSED!");
}

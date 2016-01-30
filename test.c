/*
 * pa2 tester
 * bmilton
 * 25 jan 2016
 * latest update: 26 jan 2016
 */

#include "test.h"

int totalFailCounter = 0;
int verbose = 0;
int should_run_havoc = 0;

int inSlackRange(int expected, int actual) {
  if(actual >= expected) return 1;
  if(!SLACK) return 0;
  int minimumAllowed = expected * 0.9;
  return(actual >= minimumAllowed);
}

int get_next_sched() {
  if (LOGIC_IN_HANDLETIMERINTR) {
    HandleTimerIntr();
  }
  return SchedProc();
}


int test_arbitrary(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(ARBITRARY);
  InitSched();
  int proc;
  for (proc = 1; proc <= numprocs; proc++)
    StartingProc(proc);

  for (proc = 0; proc < 500; proc++) {
    int pid = get_next_sched();
    if (!pid) {
      Printf("ARBITRARY ERR: Received bad proces %d\n", pid);
      failCounter++;
    }
  }

  for (proc = 1; proc <= numprocs; proc++)
    EndingProc(proc);

  totalFailCounter += failCounter;
  return failCounter;
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
    int next = get_next_sched();
    if (next != proc) {
      Printf("FIFO ERR: Received process %d but expected %d\n", next, proc);
      failCounter++;
    }

    EndingProc(proc);
  }

  /* check if all process are exited */
  if (get_next_sched()) {
    Printf("FIFO ERR: Not all processes have exited\n");
    failCounter++;
  }

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
    int next = get_next_sched();
    if (next != proc) {
      Printf("LIFO ERR: Received process %d but expected %d\n", next, proc);
      failCounter++;
    }

    EndingProc(proc);
  }

  /* check if all process are exited */
  if (get_next_sched()) {
    Printf("LIFO ERR: Not all processes have exited\n");
    failCounter++;
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test_rr_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(ROUNDROBIN);
  InitSched();

  for (int i = 1; i <= numprocs; i++) {
    StartingProc(i);
  }

  int* decisions = calloc(numprocs, 4);

  for(int t = 0; t < numprocs * 5; t++) {

    // have we made at least numprocs decisions yet?
    if(t >= numprocs){
      int* counts = calloc(numprocs + 1, 4);

      for(int i = 0; i < numprocs; i++) {
        counts[decisions[i]]++;
      }

      for(int i = 1; i < numprocs; i++) {
        if(counts[i] != 1){
          Printf("ROUND ROBIN ERR: Process %d received %d ticks in one round, expecting 1\n",
                 i, counts[i]);
          failCounter++;
        }
      }

      free(counts);
    }

    decisions[t % numprocs] = get_next_sched();
  }

  free(decisions);

  for (int i = 1; i <= numprocs; i++) {
    EndingProc(i);
  }

  /* check if all process are exited */
  if (get_next_sched()) {
    Printf("ROUND ROBIN ERR: Not all processes have exited\n");
    failCounter++;
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test_rr_middle_exit() {
  // this test attempts to break a 'next pointer' implementation.
  // it only runs one cycle through
  int failCounter = 0;
  SetSchedPolicy(ROUNDROBIN);
  InitSched();
  int counts[6];
  for (int i = 1; i <= 5; i++) {
    StartingProc(i);
    counts[i] = 0;
  }

  // Run each once as in standard RR
  for (int i = 1; i <= 5; i++) {
    counts[get_next_sched()]++;
  }

  // Ensure that each was run once
  for (int i = 1; i <= 5; i++) {
    if (counts[i] != 1) {
      Printf("ROUND ROBIN2 ERR: Process %d received %d ticks in one round, expected 1\n",
          i, counts[i]);
      failCounter++;
    }
    counts[i] = 0;
  }

  // End a process prematurely
  EndingProc(3);

  // Ensure one process ran twice, process 3 didnt run, everyone else ran once
  for (int i = 1; i <= 5; i++) {
    counts[get_next_sched()]++;
  }

  int twiceProcessRan = 0;
  for (int i = 1; i <= 5; i++) {
    if (i == 3) {
      if (counts[i] != 0) {
        Printf("ROUND ROBIN2 ERR: Process %d was ended but RR still selected it!\n",
            i);
        failCounter++;
      }
      continue;
    }
    if (counts[i] == 2) {
      if (twiceProcessRan) {
        Printf("ROUND ROBIN2 ERR: Process %d got 2 but expected 1 (6 ticks occured, a single process already took 2)\n", i);
        failCounter++;
      }
      else
        twiceProcessRan = 1;
    }
    else if (counts[i] != 1) {
      Printf("ROUND ROBIN2 ERR: Process %d got %d but expected 1\n", i);
      failCounter++;
    }
  }

  for (int i = 1; i <= 5; i++) {
    if (i == 3)
      continue;

    EndingProc(i);
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test_proportional_normal(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(PROPORTIONAL);
  InitSched();
  int i, j;


  int proportions[numprocs+1];
  int counts[numprocs+1];
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
    counts[get_next_sched()]++;
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
  if (get_next_sched()) {
    Printf("PROPORTIONAL ERR: Not all processes have exited\n");
    failCounter++;
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test_proportional_hog(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(PROPORTIONAL);
  InitSched();
  int i, iter;
  int counts[numprocs+1];

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
    counts[get_next_sched()]++;
  }

  if (counts[1] < (100 - (numprocs - 1))) {
    Printf("PROPORTIONAL2 ERR: Process 1 should have received %d%% CPU time but got %d%%\n",
           (100 - (numprocs - 1)), counts[1]);
    failCounter++;
  }

  for (i = 2; i <= numprocs; i++) {
    if (counts[i] > 0) {
      Printf("PROPORTIONAL2 ERR: Process %d shouldn't have received >0%% CPU time (Received %d%%) "
             "since process 1 requested 100%%. \n",
             i, counts[i]);
      failCounter++;
    }
    counts[i] = 0;
  }

  EndingProc(1);
  for (iter = 0; iter < 100; iter++) {
    counts[get_next_sched()]++;
  }

  for (i = 2; i <= numprocs; i++) {
    if (!inSlackRange(100 / (numprocs-1), counts[i])) {
      Printf("PROPORTIONAL2 ERR: Process %d was expected to receive %d%% CPU (RR after no requests"
             " for CPU were made), but received %d%%\n", i, (100/(numprocs-1)), counts[i]);
      failCounter++;
    }
    EndingProc(i);
  }

  if (get_next_sched()) {
    Printf("PROPORTIONAL2 ERR: Not all processes have exited\n");
    failCounter++;
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test_proportional_huge(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(PROPORTIONAL);
  InitSched();
  int i, iter;
  int counts[numprocs+1];

  for (i = 1; i <= numprocs; i++) {
    StartingProc(i);
    counts[i] = 0;
  }

  if (MyRequestCPUrate(1, 100) == -1) {
    Printf("PROPORTIONAL3 ERR: MyRequestCPUrate returned -1 when CPU was available\n");
    failCounter++;
  }

  for (iter = 0; iter < 50000; iter++) {
    counts[get_next_sched()]++;
  }

  if (counts[1] != 50000) {
    Printf("PROPORTIONAL3 ERR: Process 1 should have received 50000 ticks but got %d\n",
           counts[1]);
    failCounter++;
  }

  for (i = 2; i <= numprocs; i++) {
    if (counts[i]) {
      Printf("PROPORTIONAL3 ERR: Process %d received %d ticks but should have received none "
             "since process 1 requested 100%%\n", i, counts[i]);
      failCounter++;
    }
  }

  for (i = 1; i <= numprocs; i++)
    EndingProc(i);
  
  if (get_next_sched()) {
    Printf("PROPORTIONAL3 ERR: Not all processes have exited\n");
    failCounter++;
  }

  totalFailCounter += failCounter;
  return failCounter;
}

/*
 * A simple test case to ensure that any processes with no CPU allocation 
 * don't take too much CPU, even if there is less than 1% to go around. Also 
 * ensures that 1% is appropriately split among processes with no CPU 
 * allocation.
 */
int test_proportional_split_amongst_procs(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(PROPORTIONAL);
  InitSched();
  int i, iter;
  int counts[numprocs+1];
  int numRecvd = 0;

  if (verbose)
    Printf("start PROPORTIONALSPLIT with %d procs\n", numprocs);

  for (i = 1; i <= numprocs; i++) {
    StartingProc(i);
    counts[i] = 0;
  }

  if (MyRequestCPUrate(1, 99) == -1) {
    Printf("PROPORTIONALSPLIT ERR: MyRequestCPUrate returned -1 when CPU was available\n");
    failCounter++;
  }

  for (iter = 0; iter < 500; iter++) {
    counts[get_next_sched()]++;
  }

  if (!inSlackRange(495, counts[1])) {
    Printf("PROPORTIONALSPLIT ERR: Process 1 should have received at least %d CPU ticks but got %d\n",
           (int) (495 * 0.9), counts[1]);
    failCounter++;
  }

  for (i = 1; i <= numprocs; i++) {
    if (verbose)
      Printf("proc %d recvd %d ticks\n", i, counts[i]);
    if (i > 1 && counts[i] >= 1)
      numRecvd++;
  }

  if (numprocs >= 6 && numRecvd < 5) {
    Printf("PROPORTIONALSPLIT ERR:" 
        " At >5 procs should have received a cpu tick"
        " since process 1 requested 99%%\n");
    failCounter++;
  }

  for (i = 1; i <= numprocs; i++)
    EndingProc(i);

  if (get_next_sched()) {
    Printf("PROPORTIONALSPLIT ERR: Not all processes have exited\n");
    failCounter++;
  }

  totalFailCounter += failCounter;
  return failCounter;
}

/*
 * A test case to ensure one process flooding with requests doesn't allow it
 * to hog all CPU.
 */
int test_proportional_reqhog(int numprocs) {
  int failCounter = 0;
  SetSchedPolicy(PROPORTIONAL);
  InitSched();
  int i, iter;
  int counts[numprocs+1];
  int unallocCount = 0;

  if (verbose)
    Printf("start PROPORTIONALREQHOG with %d procs\n", numprocs);

  for (i = 1; i <= numprocs; i++) {
    StartingProc(i);
    counts[i] = 0;
  }

  for (iter = 0; iter < 500; iter++) {
    if (MyRequestCPUrate(1, 90) == -1) {
      Printf("PROPORTIONALREQHOG ERR: MyRequestCPUrate returned -1 when CPU was available\n");
      failCounter++;
    }

    counts[get_next_sched()]++;
  }

  if (!inSlackRange(450, counts[1])) {
    Printf("PROPORTIONALREQHOG ERR: Process 1 should have received at least %d CPU ticks but got %d\n",
           (int) (450 * 0.9), counts[1]);
    failCounter++;
  }

  for (i = 1; i <= numprocs; i++) {
    if (verbose)
      Printf("proc %d recvd %d ticks\n", i, counts[i]);
    if (i > 1)
      unallocCount += counts[i];
  }

  if (numprocs > 1 && !inSlackRange(50, unallocCount)) {
    Printf("PROPORTIONALREQHOG ERR: Processes which allocated no CPU should "
        "have received at least %d CPU ticks total but got %d\n",
           (int) (50 * 0.9), unallocCount);
    failCounter++;
  }

  for (i = 1; i <= numprocs; i++)
    EndingProc(i);

  if (get_next_sched()) {
    Printf("PROPORTIONALREQHOG ERR: Not all processes have exited\n");
    failCounter++;
  }

  totalFailCounter += failCounter;
  return failCounter;
}

int test(int (*testerFunction) (int)) {
  int i, failures;
  failures = 0;
  for (i = 1; i <= MAXPROCS; i++) {
    failures += testerFunction(i);
  }
  return failures;
}

void parseCommandLineArg(int argc, char** argv){
  for (int i = 1; i < argc; i++){
    if (!strcmp(argv[i], "--havoc"))
      should_run_havoc= 1;
    else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
      verbose = 1;
  }
}

void Main(int argc, char** argv) {

  parseCommandLineArg(argc, argv);

  srand(120 * 0xDEAD);
  Printf("%d arbitrary failures\n", test(&test_arbitrary));
  Printf("%d fifo failures\n", test(&test_fifo_normal));
  Printf("%d lifo failures\n", test(&test_lifo_normal));
  Printf("%d roundrobin failures\n", test(&test_rr_normal));
  Printf("%d roundrobin2 failures\n", test_rr_middle_exit());
  Printf("%d proportional failures\n", test(&test_proportional_normal));
  Printf("%d proportional2 failures\n", test(&test_proportional_hog));
  Printf("%d proportional3 failures\n", test(&test_proportional_huge));
  Printf("%d proportionalSPLIT failures\n", test(&test_proportional_split_amongst_procs));
  Printf("%d proportionalREQHOG failures\n", test(&test_proportional_reqhog));

  if(should_run_havoc){
    int havoc_fails;
    //              +-------------------- length of test, ticks
    //              |     +-------------- 1/P(start new process?)
    //              |     |  +----------- 1/P(end random process?)
    //              |     |  |   +------- 1/P(process storm?)
    //              |     |  |   |   +--- 1/P(request new priority?)
    // test_havoc(100000, 4, 4, 500, 0);
    
    // stress implementation of equal distribution
    havoc_fails += test_havoc(1000000, 10, 10, 500, 0);

    // stress choice of new pass value
    havoc_fails += test_havoc(1000000, 4, 4, 500, 4);
    havoc_fails += test_havoc(1000000, 0, 5, 100, 10);
    
    // test stability over a long period
    havoc_fails += test_havoc(100000, 0, 0, 2, 0);
    havoc_fails += test_havoc(100000, 0, 0, 2, 10000);

    // old default
    havoc_fails += test_havoc(1000000, 1100, 1000, 50000, 500);

    // do a lot of stressful things
    havoc_fails += test_havoc(1000000, 0, 0, 0, 0);
    havoc_fails += test_havoc(1000000, 0, 0, 0, 1);
    havoc_fails += test_havoc(1000000, 0, 0, 1, 0);
    havoc_fails += test_havoc(1000000, 0, 0, 1, 1);
    havoc_fails += test_havoc(1000000, 0, 1, 0, 0);
    havoc_fails += test_havoc(1000000, 0, 1, 0, 1);
    havoc_fails += test_havoc(1000000, 0, 1, 1, 0);
    havoc_fails += test_havoc(1000000, 0, 1, 1, 1);
    havoc_fails += test_havoc(1000000, 1, 0, 0, 0);
    havoc_fails += test_havoc(1000000, 1, 0, 0, 1);
    havoc_fails += test_havoc(1000000, 1, 0, 1, 0);
    havoc_fails += test_havoc(1000000, 1, 0, 1, 1);
    havoc_fails += test_havoc(1000000, 1, 1, 0, 0);
    havoc_fails += test_havoc(1000000, 1, 1, 0, 1);
    havoc_fails += test_havoc(1000000, 1, 1, 1, 0);
    havoc_fails += test_havoc(1000000, 1, 1, 1, 1);
    
    totalFailCounter += havoc_fails;
    Printf("%d havoc failures\n", havoc_fails);
  }

  Printf("%d Failures\n", totalFailCounter);
  if (totalFailCounter == 0)
    Printf("ALL TESTS PASSED!");
}

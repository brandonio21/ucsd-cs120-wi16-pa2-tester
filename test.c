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
#include <string.h>

#ifndef SLACK
#define SLACK 1
#endif

#ifndef LOGIC_IN_HANDLETIMERINTR
#define LOGIC_IN_HANDLETIMERINTR 0
#endif

int totalFailCounter = 0;

int inSlackRange(int expected, int actual) {
  if(actual >= expected) return 1;
  if(!SLACK) return 0;
  int minimumAllowed = expected * 0.9;
  return actual >= minimumAllowed;
}

int get_next_sched() {
  if (LOGIC_IN_HANDLETIMERINTR) {
    HandleTimerIntr();
  }
  return SchedProc();
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
    if (counts[i] > 1) {
      Printf("PROPORTIONAL2 ERR: Process %d shouldn't have received >1%% CPU time (Received %d%%) "
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

  for (iter = 0; iter < 500; iter++) {
    counts[get_next_sched()]++;
  }

  if (counts[1] < (500 - (numprocs - 1))) {
    Printf("PROPORTIONAL3 ERR: Process 1 should have received at least %d CPU ticks but got %d\n",
           (500 - (numprocs - 1)), counts[1]);
    failCounter++;
  }

  for (i = 2; i <= numprocs; i++) {
    if (counts[i] > 1) {
      Printf("PROPORTIONAL3 ERR: Process %d shouldnt have received >1 CPU tick (Recieved %d ticks) "
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

// TODO: move all of havoc into a new C file
static struct{
  int active;
  int start_tick;
  int tick_count;
  int request;
} processes[MAXPROCS+1];

static int t = 0;
static int active_procs = 0;
static int last_event = 0;
static int decision[100];
static int remaining_allocation = 100;

static int get_random_active(){
  // There are (active_procs) PIDs to pick from
  // Of these, pick the nth
  int n = rand() % active_procs;
  for(int pid = 1; pid <= MAXPROCS; pid++){
    if(!processes[pid].active) continue;
    if(!n--) return pid;
  }
  return 0;
}

static int get_random_inactive(){
  // There are (MAXPROCS - active_procs) PIDs to pick from
  // Of these, pick the nth
  int n = rand() % (MAXPROCS - active_procs);
  for(int pid = 1; pid <= MAXPROCS; pid++){
    if(processes[pid].active) continue;
    if(!n--) return pid;
  }
  return 0;
}

// start a process with pid pid; return 0 on success
static int start_process(int pid){
  if(!StartingProc(pid)) {
    Printf("HAVOC ERR: StartingProc failed\n");
    return 1;
  }

  active_procs++;
  processes[pid].active = 1;
  processes[pid].start_tick = t;

  //Printf("Started new process %d\n", pid);
  return 0;
}

// start a process with pid pid; return 0 on success
static int end_process(int pid){
  for(int i = 0; i < 100; i++){
    if(decision[i] == pid) decision[i] = 0;
  }

  if(!EndingProc(pid)) {
    Printf("HAVOC ERR: EndingProc failed\n");
    return 1;
  }

  remaining_allocation += allocated[pid];
  active_procs--;
  processes[pid].active = 0;
  processes[pid].requested = 0;

  //Printf("Ended process %d\n", pid);
  return 0;
}


int test_havoc(){
  int errors = 0;
  int last_event = 0;
  int totals[MAXPROCS+1];

  SetSchedPolicy(PROPORTIONAL);
  InitSched();

  memset(allocated, 0, sizeof(int) * (MAXPROCS + 1));
  memset(active, 0, sizeof(int) * (MAXPROCS + 1));

  for(t = 0; t < 1000000; t++) {
    // Start a new process?
    if(active_procs < MAXPROCS && !(rand() % 1100)) {
      if(start_process(get_random_inactive())) return ++errors;
      last_event = t;
    }

    // Start as many new processes as possible?
    if(!(rand() % 50000)) {
      while(active_procs < MAXPROCS) {
        if(start_process(get_random_inactive())) return ++errors;
      }
      last_event = t;
    }

    // End a random process?
    if(active_procs && !(rand() % 1000)) {
      if(end_process(get_random_active())) return ++errors;
      last_event = t;
    }

    // Change the priority of a random process?
    if(active_procs && !(rand() % 500)) {
      int pid = get_random_active();
      int max_allocation = remaining_allocation + processes[pid].requested;

      if(MyRequestCPUrate(pid, max_allocation + 1) != -1) {
        Printf("HAVOC ERR: Failed to reject overallocation request of %d for process %d\n",
               max_allocation + 1, pid);
        return ++errors;
      }

      if(max_allocation) {
        int new_allocation = 1 + rand() % max_allocation;

        // Half the time, try to allocate the maximum available first
        if(!(rand() % 2) && MyRequestCPUrate(pid, max_allocation) != 0){
          Printf("HAVOC ERR: Failed to accept valid request of %d for process %d; should have been able to request up to %d\n",
                 new_allocation, pid, max_allocation);
          return ++errors;
        }

        // Printf("Changing allocation of %d: %d -> %d\n", pid, allocated[pid], new_allocation);

        if(MyRequestCPUrate(pid, new_allocation) != 0){
          Printf("HAVOC ERR: Failed to accept valid request of %d for process %d; should have been able to request up to %d\n",
                 new_allocation, pid, max_allocation);
          return ++errors;
        }

        remaining_allocation += processes[pid].request - new_allocation;
        processes[pid].request = new_allocation;

        // We reset the start of a process when we make a new rate request
        processes[pid].start_tick = t;
        last_event = t;
      }
    }

    // if we've had more than 100 ticks since the last event,
    // validate the last 100 decisions
    if(t - last_event >= 100) {
      memset(totals, 0, sizeof(int) * (MAXPROCS + 1));

      // total the ticks that each process received
      for(int i = 0; i < 100; i++)
        totals[decision[i]]++;

      // of the leftover processes (which did not request a specific CPU rate),
      // the minimum and maximum number of ticks allocated
      int leftover_min = 100, leftover_max = 0;
      int min_leftover, max_leftover;

      for(int i = 1; i <= MAXPROCS; i++){
        if(!processes[i].active) continue;
        if(!inSlackRange(processes[i].request, totals[i])) {
          Printf("HAVOC ERR: Process %d received %d of the last 100 ticks, but requested %d\n", i, totals[i], processes[i].request);
          errors++;
        }

        // if a process didn't make any request, it's a leftover process
        if(!allocated[i]) {
          if(totals[i] < leftover_min) {
            leftover_min = totals[i];
            min_leftover = i;
          }

          if(totals[i] > leftover_max) {
            leftover_max = totals[i];
            max_leftover = i;
          }
        }
      }

      for(int pid = 1; pid <= MAXPROCS; pid++) {
        int age = t - processes[pid].start_tick;
        if(age < 100) continue;
        int expected = processes[pid].request * 100 /
          (t - processes[pid].start_tick);
        if(!inSlackRange(expected, processes[pid].tick_count)) {
          Printf("HAVOC ERR: Process %d requested %d%% but only received %d of the %d ticks since its request\n",
                 pid, processes[pid].request, processes[pid].tick_count, age);
        }
      }

      // If there was at least one leftover process, check that
      // the most-scheduled leftover process is scheduled at most
      // once more than the least-scheduled leftover process
      // across the last 100 ticks

      // This is the closest we can get to verifying that the processes
      // are scheduled equally, given our 100-tick ring buffer

      if(leftover_max && leftover_max - leftover_min > 1) {
        Printf("HAVOC ERR: Leftover processes %d and %d were not scheduled equally (received %d and %d ticks respectively)\n",
               min_leftover, max_leftover, leftover_min, leftover_max);
      }
    }

    int next = get_next_sched();
    decision[t % 100] = next;
    processes[next].tick_count++;
  }

  int pid;
  while(pid = get_random_active()) EndingProc(pid);

  totalFailCounter += errors;
  return errors;
}

int test(int (*testerFunction) (int)) {
  int i, failures;
  failures = 0;
  for (i = 1; i <= MAXPROCS; i++) {
    failures += testerFunction(i);
  }
  return failures;
}

void Main(int argc, char** argv) {
  srand(120 * 0xDEAD);
  Printf("%d fifo failures\n", test(&test_fifo_normal));
  Printf("%d lifo failures\n", test(&test_lifo_normal));
  Printf("%d roundrobin failures\n", test(&test_rr_normal));
  Printf("%d proportional failures\n", test(&test_proportional_normal));
  Printf("%d proportional2 failures\n", test(&test_proportional_hog));
  Printf("%d proportional3 failures\n", test(&test_proportional_huge));
  if (argc > 1 && strcmp(argv[1], "--havok") == 0)
    Printf("%d havoc failures\n", test_havoc(MAXPROCS));

  Printf("%d Failures\n", totalFailCounter);
  if (totalFailCounter == 0)
    Printf("ALL TESTS PASSED!");
}

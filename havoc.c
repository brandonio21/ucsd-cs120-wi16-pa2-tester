#include "test.h"
#include <stdarg.h>
#include <stdio.h>

static struct{
  int active;
  int start_tick;
  int tick_count;
  int request;
} processes[MAXPROCS+1];

static int t = 0;
static int active_procs = 0;
static int last_event = 0;
static int remaining_allocation = 100;
static int errors = 0;

static void info(char* fmt, ...) {
  if(!verbose) return;
  printf("HAVOC [%7d] INF: ", t);
  va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
  va_end (args);
  printf("\n");
}

static void assert(int check, char* fmt, ...) {
  if(check) return;
  errors++;
  printf("HAVOC [%7d] \x1b[33mERR: ", t);
  va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
  va_end (args);
  printf("\x1b[0m\n");
}

static void assert_critical(int check, char* fmt, ...) {
  if(check) return;
  printf("HAVOC [%7d] \x1b[31mFATAL: ", t);
  va_list args;
  va_start (args, fmt);
  vprintf (fmt, args);
  va_end (args);
  printf("\x1b[0m\n");
  exit(-1);
}


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

static void update_last_event(int pid) {
  last_event = t;
  if(processes[pid].request) return;
  for(pid = 1; pid <= MAXPROCS; pid++){
    if(!processes[pid].request) {
      processes[pid].tick_count = 0;
    }
  }
}

// start a process with pid pid; return 0 on success
static int start_process(int pid){
  assert_critical(StartingProc(pid), "StartingProc failed");
  active_procs++;
  processes[pid].active = 1;
  processes[pid].start_tick = t;
  processes[pid].tick_count = 0;

  update_last_event(pid);
  info("Started new process %d", pid);
  return 0;
}

// end a process with pid pid; return 0 on success
static int end_process(int pid){
  assert_critical(EndingProc(pid), "EndingProc failed");

  remaining_allocation += processes[pid].request;
  active_procs--;
  processes[pid].active = 0;
  processes[pid].request = 0;
  update_last_event(pid);
  
  info("Ended process %d after %d ticks", pid, t - processes[pid].start_tick);
  return 0;
}

int test_havoc(){
  int last_event = 0;
  int totals[MAXPROCS+1];

  SetSchedPolicy(PROPORTIONAL);
  InitSched();

  memset(processes, 0, sizeof(processes));

  for(t = 0; t < 1000000; t++) {
    // Start a new process?
    if(active_procs < MAXPROCS && !(rand() % 1100)) {
      if(start_process(get_random_inactive())) return ++errors;
    }

    // Start as many new processes as possible?
    if(!(rand() % 50000)) {
      while(active_procs < MAXPROCS) {
        if(start_process(get_random_inactive())) return ++errors;
      }
    }

    // End a random process?
    if(active_procs && !(rand() % 1000)) {
      if(end_process(get_random_active())) return ++errors;
    }

    // Change the priority of a random process?
    if(active_procs && !(rand() % 500)) {
      int pid = get_random_active();
      int max_allocation = remaining_allocation + processes[pid].request;

      assert_critical(MyRequestCPUrate(pid, max_allocation + 1) == -1,
		   "Failed to reject overallocation request of %d for process %d",
		   max_allocation + 1, pid);

      if(max_allocation) {
        int new_allocation = 1 + rand() % max_allocation;

        // Half the time, try to allocate the maximum available first
        if(!(rand() % 2))
	  assert_critical(MyRequestCPUrate(pid, max_allocation) == 0,
		       "Failed to accept valid request of %d for process %d; "
		       "should have been able to request up to %d",
		       max_allocation, pid, max_allocation);


        assert_critical(MyRequestCPUrate(pid, new_allocation) == 0,
		     "Failed to accept valid request of %d for process %d; "
		     "should have been able to request up to %d",
		     new_allocation, pid, max_allocation);


        remaining_allocation += processes[pid].request - new_allocation;
	info("Process %2d changing allocation: %3d -> %3d, new total allocation: %3d%%",
	     pid, processes[pid].request, new_allocation, 100 - remaining_allocation);

	processes[pid].request = new_allocation;
        
        // We reset the start of a process when we make a new rate request
        processes[pid].start_tick = t;
	processes[pid].tick_count = 0;

	update_last_event(pid);
      }
    }

    // of the leftover processes (which did not request a specific CPU rate),
    // the minimum and maximum number of ticks allocated
    int leftover_min = 0x7FFFFFFF, leftover_max = 0;
    int min_leftover, max_leftover;
    int leftover_count = 0;
    
    for(int i = 1; i <= MAXPROCS; i++) {
      if(!processes[i].request && processes[i].active){
	leftover_count++;
	int count = processes[i].tick_count;
	if(count < leftover_min) {
	  leftover_min = count;
	  min_leftover = i;
	}
	
	if(count > leftover_max) {
	  leftover_max = count;
	  max_leftover = i;
	}
      }
    }

    // If there were extra ticks, and it's been at least 100 ticks
    // since the last event, we expect to see at least one leftover
    // process run in that time
    /*if(remaining_allocation && (t - last_event >= MAXPROCS*100) && leftover_count && !leftover_max){
      Printf("HAVOC ERR: %d Expected a leftover process to run in the last %d ticks\n", t, MAXPROCS*100);
      }*/
	
    
    // If there was at least one leftover process, check that
    // the most-scheduled leftover process is scheduled at most
    // once more than the least-scheduled process
    assert(!leftover_count || leftover_max - leftover_min <= 1,
	   "Leftover processes %d and %d were not scheduled equally "
	   "(received %d and %d ticks respectively out of %d)",
	   min_leftover, max_leftover, leftover_min, leftover_max, t - last_event);
    
    for(int i = 1; i <= MAXPROCS; i++){
      if(!processes[i].active) continue;
      if(!processes[i].request) continue;
      int age = t - processes[i].start_tick;
      if(age < 100) continue;
      int expected = processes[i].request * age / 100;

      assert(inSlackRange(expected, processes[i].tick_count),
	     "Process %2d requested %3d%% but only received %2.2f%% "
	     "(%d of the %d ticks since its request)",
	     i, processes[i].request, 100.0 * processes[i].tick_count / age,
	     processes[i].tick_count, age);
    }
    
    processes[get_next_sched()].tick_count++;
  }

  while(active_procs) end_process(get_random_active());
  return errors;
}


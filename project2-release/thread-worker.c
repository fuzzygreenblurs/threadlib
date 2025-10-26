// File:	thread-worker.c
// List all group member's name: Akhil Sankar 
// username of iLab: 
// iLab Server:

#include "thread-worker.h"

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;

// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
static int elapsed_quantums = 0;
static long completed_threads = 0;
static long accum_turn_time = 0; 
static long accum_resp_time = 0; 

static worker_t latest_assigned_id = 0;
static tcb* runqueue_head = NULL;
static tcb* runqueue_tail = NULL;

static tcb* tracked_head = NULL;
static tcb* tracked_tail = NULL;

static ucontext_t scheduler_context;
static ucontext_t main_context; 
static tcb* current = NULL;
static int scheduler_initialized = 0;
static void schedule(void);
static void store_cb_retval(void);

/* create a new thread */
int worker_create(worker_t* thread, pthread_attr_t* attr, 
                  void*(*function)(void*), void* arg) {

  /*
   * void*(*function)(void*)
   * function          : variable name
   * *function         : a pointer
   * (*function)()     : pointer to a function
   * (*function)(void*): functikon takes one void* argument
   * void*(...)        : function returns void pointer
   *
   */
  // - allocate space of stack for this thread to run
  // after everything is set, push this thread into run queue and 
  // - make it ready for the execution.

  // YOUR CODE HERE
  // TODO: break up into individual functions. this is bloated.
  if(!scheduler_initialized) {
    scheduler_initialized = 1;

    getcontext(&scheduler_context);
    getcontext(&main_context);
    scheduler_context.uc_link = NULL;
    scheduler_context.uc_stack.ss_sp = malloc(SIGSTKSZ);
    scheduler_context.uc_stack.ss_size = SIGSTKSZ;
    scheduler_context.uc_stack.ss_flags = 0;

    makecontext(&scheduler_context, schedule, 0);

    setup_timer();
  }

  // - create Thread Control Block (TCB)
  
  // TODO: does creating the new thread need to be atomic? 
  tcb* worker_tcb = (tcb*)malloc(sizeof(tcb)); 
  if(!worker_tcb) return -1;                     // TODO: assert/raise readable error?
  worker_tcb->thread_id = latest_assigned_id++;
  worker_tcb->elapsed_time = 0;
  *thread = worker_tcb->thread_id; 

  worker_tcb->creation_time = elapsed_quantums;
  worker_tcb->first_run_time = -1;
  worker_tcb->cb = function;
  worker_tcb->cb_arg = arg;


  
  // create and initialize the context of this worker thread      
  // getcontext initializes internal fields of the gien ucontex_t struct
  // it captures the current CPU state as a starting template
  // setup proper signal masks, flags and architectture specific data
  if(getcontext(&(worker_tcb->context)) < 0) {
    free(worker_tcb); 
    return -1;
  } 

  // worker_tcb->stack is of type void*
  worker_tcb->stack = malloc(SIGSTKSZ);
  if(!worker_tcb->stack) {
    free(worker_tcb);
    return -1;
  }

  // STEP 2 TODO: notes on ss_flags, uc_link?
  worker_tcb->context.uc_link = NULL;
  worker_tcb->context.uc_stack.ss_sp = worker_tcb->stack;
  worker_tcb->context.uc_stack.ss_size = SIGSTKSZ;
  worker_tcb->context.uc_stack.ss_flags = 0;

  // TODO: verify if this is the right way to cast this function pointer
  makecontext(&(worker_tcb->context), 
              (void(*)(void))store_cb_retval,
              0);
              
  track(worker_tcb);
  enqueue(worker_tcb);
  worker_tcb->status = READY;
  return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
  if(current == NULL) {
    swapcontext(&main_context, &scheduler_context);
    return 0;
  }
  current->status = READY;
  enqueue(current);

  swapcontext(&(current->context), &scheduler_context);
	
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread

  current->retval = value_ptr;
  free(current->stack);            
  current->status = TERMINATED;
  current->completion_time = elapsed_quantums;

  accum_turn_time += current->completion_time - current->creation_time; 
  accum_resp_time += current->first_run_time  - current->creation_time;
  completed_threads++;

  avg_turn_time = (double)(accum_turn_time/completed_threads) * QUANTUM;
  avg_resp_time = (double)(accum_resp_time/completed_threads) * QUANTUM;

  setcontext(&scheduler_context);
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
  tcb* prev = NULL;
  tcb* candidate = find_tracked_thread(thread, &prev);
  if(candidate == NULL) return -1;

  while(candidate->status != TERMINATED) {
    worker_yield();
  }

  if(value_ptr != NULL) {
    *value_ptr = candidate->retval;
  }

  untrack(candidate, prev);
  return 0;
};
/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
  mutex->locked = 0;
  mutex->blocked_head = NULL;
  mutex->blocked_tail = NULL;
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {
  // -:149
  // :use the built-in test-and-set atomic function to test the mutex
  // - if the mutex is ahow me aquired successfully, enter the critical section
  // - if acquiring mutex fails, push current thread into block list and
  // context switch to the scheduler thread

  // YOUR CODE HERE
  if(__sync_lock_test_and_set(&mutex->locked, 1)) {
    current->status = BLOCKED;

    if(mutex->blocked_tail == NULL) {
      mutex->blocked_head = current;
    } else {
      mutex->blocked_tail->runqueue_next = current;
    }
    mutex->blocked_tail = current;
    current->runqueue_next = NULL;
    
    swapcontext(&(current->context), &scheduler_context);
  } 
  
  return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could coimpete for mutex later.

	// YOUR CODE HERE
  if(mutex->blocked_head != NULL) {
    tcb* unblocked = mutex->blocked_head;
    mutex->blocked_head = unblocked->runqueue_next;
    unblocked->status = READY;
    enqueue(unblocked);

    if(mutex->blocked_head == NULL) {
      mutex->blocked_tail = NULL;
    }
  }

  mutex->locked = 0;

  return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
  if(current && current->status != TERMINATED && current->status != BLOCKED) {
    current->elapsed_time += 1;
    current->status = READY;
    enqueue(current);
  }

  tcb* min_elapsed = find_min_elapsed();
  if(min_elapsed == NULL) return;
  
  if(min_elapsed->first_run_time == -1) {
    min_elapsed->first_run_time = elapsed_quantums;
  }
  remove_from_runqueue(min_elapsed);
  
  current = min_elapsed;
  current->status = SCHEDULED;
swapcontext(&scheduler_context, &current->context);
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE

	/* Step-by-step guidances */
	// Step1: Calculate the time current thread actually ran
	// Step2.1: If current thread uses up its allotment, demote it to the low priority queue (Rule 4)
	// Step2.2: Otherwise, push the thread back to its origin queue
	// Step3: If time period S passes, promote all threads to the topmost queue (Rule 5)
	// Step4: Apply RR on the topmost queue with entries and run next thread
}

/* Completely fair scheduling algorithm */
static void sched_cfs(){
	// - your own implementation of CFS
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE

	/* Step-by-step guidances */

	// Step1: Update current thread's vruntime by adding the time it actually ran
	// Step2: Insert current thread into the runqueue (min heap)
	// Step3: Pop the runqueue to get the thread with a minimum vruntime
	// Step4: Calculate time slice based on target_latency (TARGET_LATENCY), number of threads within the runqueue
	// Step5: If the ideal time slice is smaller than minimum_granularity (MIN_SCHED_GRN), use MIN_SCHED_GRN instead
	// Step5: Setup next time interrupt based on the time slice
	// Step6: Run the selected thread
}


/* scheduler */
// - invoke scheduling algorithms according to the policy (PSJF or MLFQ or CFS)
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function
	
	//YOUR CODE HERE
  while(1) {
    if(runqueue_head == NULL) {
      swapcontext(&scheduler_context, &main_context);
    }

    tot_cntx_switches++;
    #if defined(PSJF)
      sched_psjf();
    #elif defined(MLFQ)
      sched_mlfq();
    #elif defined(CFS)
      sched_cfs();  
    #else
      # error "Define one of PSJF, MLFQ, or CFS when compiling. e.g. make SCHED=MLFQ"
    #endif

    if(current != NULL && 
       current->status == TERMINATED && 
       current->joinable == 0) {
      
      tcb* prev;
      tcb* zombie = find_tracked_thread(current->thread_id,&prev);
      if(zombie) {
        untrack(zombie, prev);
        current = NULL;
      }
    }
  }
}



//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
      fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

int worker_detach(worker_t thread) { 
  tcb* candidate = find_tracked_thread(thread, NULL);
  if(candidate == NULL) return -1;

  candidate->joinable = 0;

	return 0;
}

static void enqueue(tcb *thread) {
  thread->runqueue_next = NULL;
  if(runqueue_head == NULL) {
    runqueue_head = thread;
    runqueue_tail = runqueue_head;
  } else {
    runqueue_tail->runqueue_next = thread;
    runqueue_tail = thread;
  }
}

static tcb* dequeue() {
  if(runqueue_head == NULL) {
    return NULL;
  } 

  tcb* thread = runqueue_head;
  runqueue_head = thread->runqueue_next;
 
  if(runqueue_head == NULL) {
    runqueue_tail = NULL;
  }
  
  thread->runqueue_next = NULL;
  return thread;
}

static void run_scheduler()  {
  /*
  * if no next thread to run, then swap to main
  * if next thread, swap to that one (next becomes current)
  * after swapping back, cleanup current thread if in zombie state 
  */

  while(1) {
    tcb* next = dequeue();
    if(next == NULL) {
      swapcontext(&scheduler_context, &main_context);
    } 

    current = next;
    current->status = SCHEDULED;
    swapcontext(&scheduler_context, &current->context);

    if(current != NULL && 
       current->status == TERMINATED && 
       current->joinable == 0) {
      
      tcb* prev;
      tcb* zombie = find_tracked_thread(current->thread_id,&prev);
      if(zombie) {
        untrack(zombie, prev);
        current = NULL;
      }
    }
  }
}


// 1.1.5: setup periodic interrupts to invoke scheduler

static void timer_isr() {
  elapsed_quantums++;
  if(current != NULL) {
    swapcontext(&(current->context), &(scheduler_context));
  }
}

static void setup_timer() {
  // bind ISR to SIGPROF flag
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa)); 
  sa.sa_handler = timer_isr;
  sigaction(SIGPROF, &sa, NULL); 

  struct itimerval timer;
  
  // reset to countdown value after each interval
  timer.it_interval.tv_sec = 0; 
  timer.it_interval.tv_usec = QUANTUM * 1000;

  // initial timer countdown value 
  timer.it_value.tv_sec  =  0;
  timer.it_value.tv_usec = QUANTUM * 1000;
 
  // start timer
  setitimer(ITIMER_PROF, &timer, NULL);
}

static void track(tcb* thread) {
  thread->joinable = 1;
  if(tracked_head == NULL) {
    tracked_head = thread;
    tracked_tail = tracked_head;
  } else {
    tracked_tail->tracked_next = thread;
    tracked_tail = thread;
  }

  thread->tracked_next = NULL;  
}

static void untrack(tcb* thread, tcb* prev) {
  if(thread == tracked_head) {
    if(thread->tracked_next == NULL) {
      tracked_head = NULL;
      tracked_tail = NULL;
    } else {
      tracked_head = thread->tracked_next;
    }
  } else if (thread == tracked_tail) {  
    prev->tracked_next = NULL; 
    tracked_tail = prev;
  } else {
    prev->tracked_next = thread->tracked_next;
  }

  free(thread);
}

static tcb* find_tracked_thread(worker_t thread, tcb** prev_tracker) {
  if(tracked_head == NULL) return NULL;

  tcb* candidate = tracked_head;
  tcb* prev = NULL;

  do {
    if(candidate->thread_id == thread) {
      // optional param prev_tracker can be provided
      if(prev_tracker != NULL) {
        *prev_tracker = prev;
      }

      return candidate;
    } 

    prev = candidate;
    candidate = candidate->tracked_next;
  } while(candidate != NULL);

	return NULL;
}

static tcb* find_min_elapsed() {
  int min_elapsed = INT_MAX;
  tcb* min_thread = NULL;

  tcb* candidate = runqueue_head;
  while(candidate != NULL) {
    if(candidate->elapsed_time < min_elapsed) {
      min_elapsed = candidate->elapsed_time;
      min_thread = candidate;
    }

    candidate = candidate->runqueue_next;
  }

  return min_thread;
}

static void remove_from_runqueue(tcb* thread) {
  if(runqueue_head == NULL) return;
  
  if(thread == runqueue_head) {
    runqueue_head = runqueue_head->runqueue_next;
    if(runqueue_head == NULL) { 
      runqueue_tail = NULL;
    }
    thread->runqueue_next = NULL;
    return;
  }

  tcb* candidate = runqueue_head;
  tcb* prev = NULL;

  do {
    if(candidate == thread) {
      if(candidate == runqueue_tail) {
        prev->runqueue_next = NULL;
        runqueue_tail = prev;
      } else {
        prev->runqueue_next = thread->runqueue_next;
      } 

      thread->runqueue_next = NULL;
      return;
    } 

    prev = candidate;
    candidate = candidate->runqueue_next;
  } while(candidate != NULL);
}

static void store_cb_retval(void) {
  void* ret = current->cb(current->cb_arg);
  worker_exit(ret);
}

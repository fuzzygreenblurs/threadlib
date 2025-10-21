// File:	thread-worker.c
// List all group member's name:
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
static worker_t latest_assigned_id = 0;
static tcb* queue_head = NULL;
static tcb* queue_tail = NULL;

static ucontext_t scheduler_context;
static ucontext_t main_context; 
static tcb* current = NULL;
static int scheduler_initialized = 0;
/* create a new thread */
int worker_create(worker_t* thread, pthread_attr_t* attr, 
                  void* (*function)(void*), void* arg) {

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
    scheduler_context.uc_link = NULL;
    scheduler_context.uc_stack.ss_sp = malloc(SIGSTKSZ);
    scheduler_context.uc_stack.ss_size = SIGSTKSZ;
    scheduler_context.uc_stack.ss_flags = 0;

    makecontext(&scheduler_context, run_scheduler, 0);

    setup_timer();
  }

  // - create Thread Control Block (TCB)
  
  // TODO: does creating the new thread need to be atomic? 
  tcb* worker_tcb = (tcb*)malloc(sizeof(tcb)); 
  if(!worker_tcb) return -1;                     // TODO: assert/raise readable error?
  worker_tcb->thread_id = latest_assigned_id++;
  *thread = worker_tcb->thread_id; 
       
  // - create and initialize the context of this worker thread      
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
              (void (*)(void))function,
              1, arg);
              
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
	
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE
};


/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE
	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could coimpete for mutex later.

	// YOUR CODE HERE
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
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function
	
	//YOUR CODE HERE

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ or CFS)
#if defined(PSJF)
    	sched_psjf();
#elif defined(MLFQ)
	sched_mlfq();
#elif defined(CFS)
    	sched_cfs();  
#else
	# error "Define one of PSJF, MLFQ, or CFS when compiling. e.g. make SCHED=MLFQ"
#endif
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

static void enqueue(tcb *thread) {
  thread->next = NULL;
  if(queue_head == NULL) {
    queue_head = thread;
    queue_tail = queue_head;
  } else {
    queue_tail->next = thread;
    queue_tail = thread;
  }
}

static tcb* dequeue() {
  if(queue_head == NULL) {
    return NULL;
  } 

  tcb* thread = queue_head;
  queue_head = thread->next;
 
  if(queue_head == NULL) {
    queue_tail = NULL;
  }

  return thread;
  }
}

static void run_scheduler()  {
  while(1) {
    tcb* next = dequeue();
    if(next == NULL) {
      swapcontext(&scheduler_context, &main_context);
    } 

    current = next;
    current->status = SCHEDULED;
    swapcontext(&scheduler_context, &current->context);
  }
}


// 1.1.5: setup periodic interrupts to invoke scheduler

static void timer_isr() {
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



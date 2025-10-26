// File:	worker_t.h

// List all group member's name:
// username of iLab:
// iLab Server:
#define _XOPEN_SOURCE 600
#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* Targeted latency in milliseconds */
#define TARGET_LATENCY   20  

/* Minimum scheduling granularity in milliseconds */
#define MIN_SCHED_GRN    1

/* Time slice quantum in milliseconds */
#define QUANTUM 10

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>   // For memset
#include <limits.h>   // For INT_MAX
#define READY      0
#define SCHEDULED  1
#define BLOCKED    2 
#define TERMINATED 3

typedef int worker_t;
typedef struct TCB {
  worker_t    thread_id;
  int         status;
  ucontext_t  context;
  void*       stack;
  struct TCB* runqueue_next; 
  struct TCB* tracked_next;
  int         joinable;
  int         elapsed_time;
  void*       retval;

  void*       (*cb)(void*);
  void*       cb_arg;

  int         creation_time;
  int         first_run_time;
  int         completion_time;
} tcb; 

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */

	// YOUR CODE HERE
  int locked;
  tcb* blocked_head;
  tcb* blocked_tail;

} worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE
  static void enqueue(tcb* thread);
  static tcb* dequeue();
  static void track(tcb* thread);
  static void untrack(tcb* thread, tcb* prev);
  static tcb* find_tracked_thread(worker_t thread, tcb* *prev_tracker);
  static void run_scheduler();
  static void setup_timer();
  static tcb* find_min_elapsed();
  static void remove_from_runqueue(tcb* thread);

/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr,
    void*(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);


/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create(t, a, f, arg) worker_create(t, a, (void*(*)(void*))f, arg)
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif

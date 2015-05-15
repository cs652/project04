#include <debug.h>
#include <random.h>
#include <stdbool.h>
#include "stdio.h"
#include <stdint.h>
#include <string.h>

#include "../devices/gpio.h"
#include "../devices/framebuffer.h"
#include "../devices/serial.h"
#include "../devices/timer.h"
#include "../devices/video.h"
#include "interrupt.h"
#include "init.h"
#include "palloc.h"
#include "malloc.h"
#include "synch.h"
#include "thread.h"
#include "vaddr.h"

/* -ul: Maximum number of pages to put into palloc's user pool. */
static size_t user_page_limit = SIZE_MAX;

/* Tasks for the Threads. */
static struct semaphore task_sem;

static void wait_pizza_test(void *);

struct wait_node {
  struct lock mutex;
  struct condition cv;
  bool done;
};

static struct wait_node sync_node;

static void t_wait(struct wait_node *wn);
static void t_exit(struct wait_node *wn);
static void prepare_pizza_test(void *);

/*
 * kernel.c
 *
 *  Created on: Oct 22, 2014
 *      Author: jcyescas
 */

static void test_swi_interrupt() {
  unsigned short green = 0x7E0;
  SetForeColour(green);
  generate_swi_interrupt(); // Function defined in interrupts.s
}

/* Initializes the Operating System. The interruptions have to be disabled at entrance.
 *
 *  - Sets interruptions
 *  - Sets the periodic timer
 *  - Set the thread functionality
 */
void init() {

  /* Initializes ourselves as a thread so we can use locks,
   then enable console locking. */

  thread_init();
  /* Initializes the frame buffer and console. */
  framebuffer_init();
  serial_init();
  video_init();

  printf("\nOsOs Kernel Initializing");

  /* Initialize memory system. */
  palloc_init(user_page_limit);
  malloc_init();

  /* Initializes the Interrupt System. */
  interrupts_init();
  timer_init();

  timer_msleep(5000000);

  /* Starts preemptive thread scheduling by enabling interrupts. */
  thread_start();

  printf("\nFinish booting.");

  /* Initialize the task_sem to coordinate the main thread with workers */

  sema_init(&task_sem, 0);

  thread_create("Welcome", PRI_MAX, &wait_pizza_test, NULL);
  sema_down(&task_sem);

  lock_init(&sync_node.mutex);
  cond_init(&sync_node.cv);
  sync_node.done = false;

  thread_create("Prepare_Pizza ", PRI_MIN, &prepare_pizza_test, NULL);
  t_wait(&sync_node);

  printf("\nAll done.");
  thread_exit();
}

//Start thread_wait
static void child_test(void *aux) {
  printf("\n");
  printf("\nBaking Bread ................\n");
  timer_msleep(1000000);
  printf("\nBaked Bread................\n");
  printf("\n");

}

static void cook_pizza_test(void *aux) {
  printf("\n Start Making Pizza ..................\n");
  tid_t cthread = thread_create("BREAD", PRI_DEFAULT, &child_test, NULL);
  thread_create("BREAD_LOW", PRI_MIN, &child_test, NULL);
  thread_create("BREAD_HIGH", PRI_DEFAULT +1, &child_test, NULL);

  printf("\n Prepared Toping .................\n");

  printf("\n Waiting for Bread ...................\n");

  thread_wait(cthread);

  printf("\nCooking Pizza With Toping...................\n");
  printf("\nPiza Done ....\n");

  printf("\n");

}

static void wait_pizza_test(void *aux) {
  printf("\n");
  printf("Hello from OsOS\n");
  tid_t pthread = thread_create("Make_pizza", PRI_MAX, &cook_pizza_test, NULL);
  printf("\n");
  sema_up(&task_sem);
}


static void prepare_pizza_test(void *aux) {
  printf("\n");
  printf("\nPreparing Pizza........................... \n");
  timer_msleep(1000000);
  printf("\n");
  t_exit(&sync_node);
}


//End thread_wait



static void t_wait(struct wait_node *wn) {
  lock_acquire(&wn->mutex);
  while (!wn->done) {
    cond_wait(&wn->cv, &wn->mutex);
  }
  lock_release(&wn->mutex);
}

static void t_exit(struct wait_node *wn) {
  lock_acquire(&wn->mutex);
  wn->done = true;
  cond_signal(&wn->cv, &wn->mutex);
  lock_release(&wn->mutex);
}


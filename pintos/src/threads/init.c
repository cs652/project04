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
static void start_shell(void *);

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
  timer_msleep(1000000);

  /* Starts preemptive thread scheduling by enabling interrupts. */
  thread_start();

  printf("\nFinish booting.");

  /* Initialize the task_sem to coordinate the main thread with workers */

  /*
   //Start thread_wait
   tid_t wthread = thread_create("Welcome", PRI_MAX, &wait_pizza_test, NULL);
   thread_wait(wthread);
   //End thread_wait
   */

  sema_init(&task_sem, 0);
  tid_t wthread = thread_create("Shell", PRI_MAX, &start_shell, NULL);
  thread_wait(wthread);
//  thread_exit();

}

static void dummy_thread(void *aux) {
//  while (1) {
//
//  }
}

static void test_my_ass(void *aux) {
  //Your code goes here
}

static void running_thread_stat(void *aux) {
  printf("\n    Name            Time  \n");
  thread_create("Dummy_Thread1", PRI_MAX, &dummy_thread, NULL);
  thread_create("Dummy_Thread2", PRI_MAX, &dummy_thread, NULL);
  thread_create("Dummy_Thread3", PRI_MAX, &dummy_thread, NULL);
  timer_msleep(1000000);
  struct list_elem *e;
  struct thread *t = thread_current();
//  struct list all_list = get_all_threads();
//  while (!list_empty(&all_list)) {
//  for (e = list_begin(&all_list); e != list_end(&all_list); e = list_next(e)) {
//    t = list_entry(list_pop_front(&all_list), struct thread, elem);
//    if (t->status == THREAD_RUNNING) {
    printf("%s----------->%d \n", t->name, t->running_ticks);
//    }
//  }
  timer_msleep(1000000);
  printf("Done");
}

static void help(void *aux) {
  printf("\n ts - thread status - show running threads (and their run time)\n");
  printf("\n run <func> - launch a thread function and wait for completion.\n");
  printf("\n bg <func> - launch a command in the background \n");
  printf("\n exit - launch a command in the background \n");
}

static void start_shell(void *aux) {
  while (1) {
    printf("\n#######################(Before)###################\n");
    char buff[20];
    int index = 0;
    char input = uart_getc();
    while (index < 19 && input != 'q') {
      printf("%c", input);
      buff[index++] = input;
      input = uart_getc();
    }
    buff[index] = '\n';
    printf("\n######################(After -- %s)####################\n", buff);

    tid_t process_to_run;
    if (buff[0] == 't' && buff[1] == 's') {
      process_to_run = thread_create("Running_Threads", PRI_MAX,
          &running_thread_stat, NULL);
    } else if (buff[0] == 'r' && buff[1] == 'u' && buff[2] == 'n') {
      process_to_run = thread_create("Run_Function", PRI_MAX,
          &running_thread_stat, NULL);
      thread_wait(process_to_run);
    } else if (buff[0] == 'b') {
      process_to_run = thread_create("Running_Threads", PRI_MAX,
          &running_thread_stat, NULL);

    } else if (buff[0] == 'h' && buff[1] == 'e' && buff[2] == 'l'
        && buff[3] == 'p') {
      process_to_run = thread_create("Help", PRI_MAX, &help, NULL);

    } else if (buff[0] == 'e' && buff[1] == 'x' && buff[2] == 'i'
        && buff[3] == 't') {
      break;
    } else if (buff[0] == 'a' && buff[1] == 's' && buff[2] == 's') {
      process_to_run = thread_create("test_my_ass", PRI_MAX, &test_my_ass, NULL);

    } else {
      printf("\n Invalid Input! \n");
      start_shell(NULL);
    }
  }
}
//Start thread_wait
static void prepare_bread_test(void *aux) {
  printf("\n");
  printf("\nBaking Bread ................\n");
  timer_msleep(1000000);
  printf("\nBaked Bread................\n");
  printf("\n");

}

static void cook_pizza_test(void *aux) {
  printf("\n Start Making Pizza ..................\n");
  tid_t cthread = thread_create("BREAD", PRI_DEFAULT, &prepare_bread_test,
  NULL);
  thread_create("BREAD_LOW", PRI_MIN, &prepare_bread_test, NULL);
  thread_create("BREAD_HIGH", PRI_DEFAULT + 1, &prepare_bread_test, NULL);

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
  thread_wait(pthread);
  printf("\n");
//  sema_up(&task_sem);
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

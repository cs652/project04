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

#define COMMAND_BUFFER_SIZE 128

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

//static void support_non_busy_test(void *param) {
//  int i = 0;
//  for (; i <= 600; i++) {
//    printf("---%d---", i);
//  }
//}

static void busy_test(void *param) {
    timer_msleep(9000000);
}

static void non_busy_test(void *param) {
    my_timer_msleep(9000000);
}
/* Initializes the Operating System. The interruptions have to be disabled at entrance.
 *
 *  - Sets interruptions
 *  - Sets the periodic timer
 *  - Set the thread functionality
 */

static void dummy_thread(void *aux) {
    int i = 0;
    while (i++ < 5000) {

    }
}

static void running_thread_stat(void *aux) {
    printf("\n");
    //  thread_create("Dummy_Thread1", PRI_MAX, &dummy_thread, NULL);
    show_running_thread_status();
}

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
    //added by yi
    keyboard_init();

    //  timer_msleep(1000000);

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

    timer_msleep(1000000);
    //  thread_create("Hello", PRI_MAX, &non_busy_test, NULL);
    tid_t wthread = thread_create("Shell", PRI_MAX, &start_shell, NULL);
    thread_wait(wthread);
    //  thread_exit();

}

static int factorial(int number) {
    if (number == 0 || number == 1) {
        return 1;
    }

    return number * factorial(number - 1);
}

static void fact_func(void *param) {
    unsigned short blue = 0x1f;
    unsigned short green = 0x7E0;

    int i = 1;
    while (i++ < 250) {
        int number = i % 25;
        int fac1 = factorial(number);
        int fac2 = factorial(number);

        ASSERT(fac1 == fac2);
        SetForeColour(green + blue);
        printf("\n%s - Factorial(%d) = %d", thread_current()->name, number, fac1);
        printf("Osos: $");
    }
}

void setPriority(char* fname, int start_index) {
    if (fname[start_index] == '-' && fname[start_index + 1] == 'p') {
        enable_priority();
    } else if (fname[start_index] == '-' && fname[start_index + 1] == 'n'
            && fname[start_index + 2] == 'p') {
        disable_priority();
    } else {
        printf("\n Unknown Priority Mode!!! \n ");
    }
}

static void run_func(void *aux) {
    char *fname = aux;
    bool is_run = true;
    int start_index = 4;
    void *func;

    if (fname[0] == 'b' && fname[1] == 'g') {
        is_run = false;
        start_index = 3;
    }

    if (fname[start_index] == 't' && fname[start_index + 1] == 'w'
            && fname[start_index + 2] == 'a' && fname[start_index + 3] == 'i'
            && fname[start_index + 4] == 't') {
        tid_t wthread = thread_create("Pizza", PRI_MAX, &wait_pizza_test, NULL);
        setPriority(fname, 10);
        if (is_run) {
            thread_wait(wthread);
        }
    } else if (fname[start_index] == 'f' && fname[start_index + 1] == 'a'
            && fname[start_index + 2] == 'c' && fname[start_index + 3] == 't') {
        tid_t wthread = thread_create("fact_func", PRI_MAX, &fact_func, NULL);
        //    setPriority(fname, 10);
        //    if (is_run) {
        thread_wait(wthread);
        //    }
    } else if (fname[start_index] == 'n' && fname[start_index + 1] == 'b'
            && fname[start_index + 2] == 's' && fname[start_index + 3] == 'y') {
        thread_create("nbsy", PRI_MAX, &non_busy_test, NULL);
        //    timer_msleep(2000000);
        tid_t wthread = thread_create("Thread_Stats", PRI_MAX, &running_thread_stat, NULL);
        setPriority(fname, 10);
        if (is_run) {
            thread_wait(wthread);
        }
    } else if (fname[start_index] == 'b' && fname[start_index + 1] == 'u'
            && fname[start_index + 2] == 's' && fname[start_index + 3] == 'y') {
        thread_create("busy", PRI_MAX, &busy_test, NULL);
        //    timer_msleep(2000000);
        tid_t wthread = thread_create("Thread_Stats", PRI_MAX, &running_thread_stat, NULL);
        setPriority(fname, 10);
        if (is_run) {
            thread_wait(wthread);
        }
    } else {
        printf("\n Unknown Function Name!!! \n ");
        return;
    }
    disable_priority();

}

static void help(void *aux) {
    printf("\n ts - thread status - show running threads (and their run time)\n");
    printf("\n run <func> - launch a thread function and wait for completion.\n");
    printf("\n bg <func> - launch a command in the background \n");
    printf("\n help - launch a commands list \n");
}

static void start_shell(void *aux) {
    unsigned short blue = 0x1f;
    unsigned short green = 0x7E0;
    SetForeColour(green);
    printf("\n#######################(Shell)###################\n");
    help(NULL);
    printf("\n#######################(Shell)###################\n");
    while (1) {
        printf("\nOsos$: ");

        char buff[20];
        buff[10] = '-';
        buff[11] = 'n';
        buff[11] = 'p';

        int index = 0;
        char input = uart_getc();
        while (index < 19 && input != 'q') {
            printf("%c", input);
            buff[index++] = input;
            input = uart_getc();
        }
        buff[index] = '\n';

        SetForeColour(green + blue);

        tid_t process_to_run;
        if (buff[0] == 't' && buff[1] == 's') {
            process_to_run = thread_create("Thread_Stats", PRI_MAX,
                    &running_thread_stat, NULL);
            thread_wait(process_to_run);
        } else if (buff[0] == 'r' && buff[1] == 'u' && buff[2] == 'n') {
            process_to_run = thread_create("Run_Function", PRI_MAX, &run_func, buff);
            thread_wait(process_to_run);
        } else if (buff[0] == 'b' && buff[1] == 'g') {
            process_to_run = thread_create("Bg_Function", PRI_MAX, &run_func, buff);
        } else if (buff[0] == 'h' && buff[1] == 'e' && buff[2] == 'l'
                && buff[3] == 'p') {
            start_shell(NULL);
        } else if (buff[0] == 'e' && buff[1] == 'x' && buff[2] == 'i'
                && buff[3] == 't') {
            break;
        } else {

            printf("\n \n !!!!!!!!( Invalid Input )!!!!!!!! \n\n");
            start_shell(NULL);
        }
        SetForeColour(green);
        printf("\nOsos$: ");
    }
}
//Start thread_wait
static void prepare_bread_test(void *aux) {
    printf("\n");
    printf("\nBaking Bread ..(%s)..............\n", thread_current()->name);
    timer_msleep(1000000);
    printf("\nBaked Bread (%s)................\n", thread_current()->name);
    printf("\n");

}

static void cook_pizza_test(void *aux) {
    printf("\n Start Making Pizza ..................\n");
    thread_create("BREAD", PRI_DEFAULT, &prepare_bread_test,
            NULL);
    tid_t cthread = thread_create("BREAD_LOW", PRI_MIN, &prepare_bread_test, NULL);
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


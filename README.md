Shell:

	Shell will allow to launch threads and optionally wait for their completion

	Commands list

		ts - thread status - show running threads (and their run time)
		run <func> - launch a thread function and wait for completion (you will need thread_wait()).
		bg <func> - launch a command in the background
		help - show these command

	and press 'q' to execute the command.

Thread Wait - thread_wait(tid_t tid):

	That is once a thread has been created, you can wait for it's completion. 
	
	Run in Shell:
		run twait -np (-np is for non priority)
		run twait -p  (-p is for priority)

	Code:
		1. created wait_list in thread.h
		2. implemented thread_wait() in thead.c
		3. moved the threads in wait_list to read_list in thread_exit()

Non Busy Wait Sleep:
	
	Shows that another thread can make more progress while waiting thread is delayed.

	Run in Shell:
		run nbsy (To run non Busy Wait implementation of sleep)
		run busy (To run Busy Wait implementation of sleep)

	Code:
		1. created semaphore sleep_sema in timer.c
		2. implemented my_timer_sleep() as non busy wait function
		3. sema_down(&sleep_sema) inside the while loop in  my_timer_sleep() to block the thread.
		4. sema_up(&sleep_sema) inside the timer_irq_handler to wake up the blocked thread.

Priority scheduling:

	Modified the scheduler so that higher priority threads run before lower priority threads.

	Run in Shell:
		run twait -p  (-p is for priority)

	Code:
		1. created is_priority variable in thread.c
		2. implemented thread_get_next_priority_thread_to_run() to get the next thread to run based on priority
		3. sorted the priority list based on the thread priority value and returned the first thread in the list.


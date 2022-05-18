#include "_test/scheduler_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static tid_t test_exec_last_tid;
static unsigned int test_exec_status;
static tid_t test_tid_13_1;
static tid_t test_tid_13_2;

#define SO_TEST_AND_SET(expect_id, new_id) \
	do { \
		if (equal_tids((expect_id), INVALID_TID) || \
				equal_tids((new_id), INVALID_TID)) \
			fprintf(stderr, "invalid task id"); \
		if (!equal_tids(test_exec_last_tid, (expect_id))) \
			fprintf(stderr, "invalid tasks order"); \
		test_exec_last_tid = (new_id); \
	} while (0)

static void test_sched_handler_13_2(unsigned int dummy)
{
	SO_TEST_AND_SET(test_tid_13_1, test_tid_13_2);
	fprintf(stderr, "%ld\n", test_exec_last_tid);
    so_exec();
	SO_TEST_AND_SET(test_tid_13_2, test_tid_13_2);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
	SO_TEST_AND_SET(test_tid_13_1, test_tid_13_2);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
	SO_TEST_AND_SET(test_tid_13_2, test_tid_13_2);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
	SO_TEST_AND_SET(test_tid_13_1, test_tid_13_2);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
	test_exec_status = SO_TEST_SUCCESS;
}

static void test_sched_handler_13_1(unsigned int dummy)
{
	test_exec_last_tid = test_tid_13_1 = get_tid();
    fprintf(stderr, "s-a intamplat?%ld\n", test_exec_last_tid);
	test_tid_13_2 = so_fork(test_sched_handler_13_2, 0);
    fprintf(stderr, "s-a intamplat?%ld\n", test_tid_13_2);
	/* allow the other thread to init */
	sched_yield();
    fprintf(stderr, "s-a intamplat?\n");
	/* I should continue running */
	SO_TEST_AND_SET(test_tid_13_1, test_tid_13_1);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
	SO_TEST_AND_SET(test_tid_13_2, test_tid_13_1);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
	SO_TEST_AND_SET(test_tid_13_1, test_tid_13_1);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
	SO_TEST_AND_SET(test_tid_13_2, test_tid_13_1);
    fprintf(stderr, "%ld\n", test_exec_last_tid);
	so_exec();
    fprintf(stderr, "s-a intamplat?\n");
	/* make sure nobody changed it until now */
	test_exec_status = SO_TEST_FAIL;
}

void test_sched_13(void)
{
	test_exec_status = SO_TEST_FAIL;

	/* quantum is 2, so each task should be preempted
	 * after running two instructions
	 */
	so_init(2, 0);

	so_fork(test_sched_handler_13_1, 0);

	sched_yield();
	so_end();
    printf("%d\n", test_exec_status);
}


#define SO_DEV0		0
#define SO_DEV1		1
#define SO_DEV2		2
#define SO_DEV3		3

#define SO_PREEMPT_UNITS	3

static unsigned int exec_time;
static unsigned int exec_devs;
static unsigned int last_priority;
static unsigned int exec_priority;
static unsigned int test_exec_status = SO_TEST_FAIL;

/*
 * 19) Test IO devices
 *
 * tests if the scheduler properly uses the IO devices
 */
static void test_sched_handler_19(unsigned int prio)
{
	if (prio != exec_priority)
		printf("invalid exec priority");

	if (so_wait(exec_devs) == 0)
		printf("invalid waiting device");

	if (so_signal(exec_devs) >= 0)
		printf("invalid signalling device");

	if (so_signal(SO_DEV0) != 0)
		printf("too many threads woken");

	test_exec_status = SO_TEST_SUCCESS;
}

void test_sched_19(void)
{
	test_exec_status = SO_TEST_FAIL;
	exec_devs = get_rand(1, SO_MAX_UNITS);
	exec_priority = get_rand(1, SO_MAX_PRIO);

	if (so_init(SO_MAX_UNITS, exec_devs) < 0) {
		so_error("initialization failed");
		goto test;
	}

	if (so_fork(test_sched_handler_19, exec_priority) == INVALID_TID) {
		so_error("cannot create new task");
		goto test;
	}

test:
	sched_yield();
	so_end();
}


static void test_sched_handler_20_signal(unsigned int dummy)
{
	unsigned int step;

	/* check if wait was called */
	if (exec_time != 2) {
		printf("thread didn't execute wait\n");
		return;
	}

	/* keep the other thread waiting longer than the preemption time */
	for (step = 0; step <= SO_PREEMPT_UNITS; step++) {
		exec_time++;
		so_exec();
	}

	/* if executed before signal, fail */
	if (test_exec_status == SO_TEST_SUCCESS)
		test_exec_status = SO_TEST_FAIL;
    printf("wut?\n");
	/* finally release the waiting thread */
	so_signal(SO_DEV0);
}

static void test_sched_handler_20_wait(unsigned int dummy)
{
	exec_time++;
	so_fork(test_sched_handler_20_signal, 0);
	exec_time++;
	so_wait(SO_DEV0);

	/* check if I waited more than preemption time */
	if (exec_time < SO_PREEMPT_UNITS + 2) {
		printf("scheduled while waiting\n");
		return;
	}

	so_exec();
    printf("de ce?\n");
	test_exec_status = SO_TEST_SUCCESS;
}

/* tests the IO functionality */
void test_sched_20(void)
{
	/* ensure that the thread gets to execute wait */
	so_init(SO_PREEMPT_UNITS, 1);

	so_fork(test_sched_handler_20_wait, 0);

	sched_yield();
	so_end();

    printf("%d\n", test_exec_status);
}




#define FAIL_IF_NOT_PRIO(prio, msg) \
	do { \
		if ((prio) != last_priority) { \
			test_exec_status = SO_TEST_FAIL; \
			printf(msg); \
		} \
		last_priority = priority; \
	} while (0)

/*
 * Threads are mixed to wait/signal lower/higher priorities
 * P2 refers to the task with priority 2
 */
static void test_sched_handler_21(unsigned int priority)
{
	switch (priority) {
	case 1:
		/* P2 should be the one that executed last */
		FAIL_IF_NOT_PRIO(2, "should have been woke by P2");
		if (so_signal(SO_DEV3) == 0)
			printf("dev3 does not exist\n");
		so_exec();
		FAIL_IF_NOT_PRIO(1, "invalid preemption");
		if (so_signal(SO_DEV0) != 2)
			printf("P1 should wake P3 and P4 (dev0)");
		FAIL_IF_NOT_PRIO(2, "preempted too early");
		if (so_signal(SO_DEV1) != 1)
			printf("P1 should wake P3 (dev1)");
		FAIL_IF_NOT_PRIO(2, "woke by someone else");
		if (so_signal(SO_DEV0) != 1)
			printf("P1 should wake P4 (dev0)");
		FAIL_IF_NOT_PRIO(4, "should be the last one");
		so_exec();
		FAIL_IF_NOT_PRIO(1, "someone else was running");
		break;

	case 2:
		last_priority = 2;
		/* wait for dev 3 - invalid device */
		if (so_wait(SO_DEV3) == 0)
			printf("dev3 does not exist");
		/* spawn all the tasks */
		so_fork(test_sched_handler_21, 4);
		so_fork(test_sched_handler_21, 3);
		so_fork(test_sched_handler_21, 1);
		so_exec();
		so_exec();

		/* no one should have ran until now */
		FAIL_IF_NOT_PRIO(2, "somebody else ran before P2");
		if (so_wait(SO_DEV1) != 0)
			printf("cannot wait on dev1");
		FAIL_IF_NOT_PRIO(3, "should run after P3");
		if (so_wait(SO_DEV2) != 0)
			printf("cannot wait on dev2");
		FAIL_IF_NOT_PRIO(3, "only P3 could wake me");
		so_exec();
		break;

	case 3:
		if (so_wait(SO_DEV0) != 0)
			printf("P3 cannot wait on dev0");
		FAIL_IF_NOT_PRIO(4, "priority order violated");
		if (so_wait(SO_DEV1) != 0)
			printf("P3 cannot wait on dev1");
		FAIL_IF_NOT_PRIO(1, "someone else woke P3");
		if (so_signal(SO_DEV2) != 1)
			printf("P3 should wake P2 (dev2)");
		break;

	case 4:
		if (so_wait(SO_DEV0) != 0)
			printf("P4 cannot wait on dev0");
		FAIL_IF_NOT_PRIO(1, "lower priority violation");
		if (so_signal(SO_DEV1) != 1)
			printf("P4 should wake P2 (dev1)");
		if (so_wait(SO_DEV0) != 0)
			printf("P4 cannot wait on dev0");
		FAIL_IF_NOT_PRIO(1, "someone else woke dev0");
		break;
	}
}

/* tests the IO and priorities */
void test_sched_21(void)
{
	test_exec_status = SO_TEST_SUCCESS;

	so_init(1, 3);

	so_fork(test_sched_handler_21, 2);

	sched_yield();
	so_end();
    printf("%d\n", test_exec_status);
}


int main() {
    test_sched_21();
    return 0;
}
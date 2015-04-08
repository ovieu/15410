#include <stdlib.h>
#include <context_switch.h>
#include <proc.h>
#include <syscall.h>
#include <simics.h>
#include <scheduler.h>
#include <asm.h>
#include <cr.h>
#include <driver.h>
#include <common_kern.h>
#include <vm.h>
#include <assert.h>

linklist_t scheduler_queue;
linklist_t sleep_queue;

#define DESCHEDULED_HT_SIZE 100
hashtable_t descheduled_tids;


typedef struct sleep_info {
    int tid;
    unsigned wake_ticks;
} sleep_info_t;

/** @brief Compares the wake times of sleep_info structs.  See linklist.h.
 *
 *  @param info1 First sleep_info_t.
 *  @param info2 Second sleep_info_t.
 *  @return A negative value of info1 should wake before info2, a positive
 *  value if info2 should wake before info1, 0 if they should wake at the same
 *  time.
 */
static int sleep_info_cmp(void *info1, void *info2) {
    return ((sleep_info_t *)info1)->wake_ticks -
           ((sleep_info_t *)info2)->wake_ticks;
}

/** @brief Initializes the scheduler.
 *
 *  @return 0 on success, negative error code otherwise.
 */
int scheduler_init()
{
    if (linklist_init(&scheduler_queue) < 0) {
        return -1;
    }
    if (linklist_init(&sleep_queue) < 0) {
        return -2;
    }
    if (hashtable_init(&descheduled_tids, DESCHEDULED_HT_SIZE) < 0) {
        return -3;
    }

    return 0;
}

/** @brief Scheduler timer interrupt handler.
 *
 *  @param ticks The number of ticks since the kernel began running.
 *  @return Void.
 */
void scheduler_tick(unsigned ticks)
{
    // TODO add linklist_get method to use instead
    // Wake sleeping threads
    sleep_info_t *sleep_info;
    while ((linklist_peek_head(&sleep_queue, (void **)&sleep_info) == 0) &&
                                        sleep_info->wake_ticks <= ticks) {
        make_runnable(sleep_info->tid);
        assert (linklist_remove_head(&sleep_queue, NULL) == 0);
    }

    int tid;
    if (linklist_remove_head(&scheduler_queue, (void**)&tid) < 0) {
        assert(gettid() == idle_tid);
        return;
    }
    linklist_add_tail(&scheduler_queue, (void*)tid);
    context_switch(tid);
}

/** @brief Context switches to the thread with ID tid.
 *
 *  Doesn't return until context switched back to.
 *
 *  @param tid The tid of the thread to context switch to.
 *  @return 0 on success, negative error code otherwise.
 */
int context_switch(int tid)
{
    if (tid == gettid()) {
        return 0;
    }
    tcb_t *new_tcb;
    if (hashtable_get(&tcbs, tid, (void**)&new_tcb) < 0) {
        return -1;
    }

    tcb_t *old_tcb;
    assert(hashtable_get(&tcbs, gettid(), (void**)&old_tcb) == 0);

    disable_interrupts();

    if (store_regs(&old_tcb->regs, old_tcb->esp0)) {
        cur_tid = new_tcb->tid;
        set_esp0(new_tcb->esp0);
        restore_regs(&new_tcb->regs, new_tcb->esp0);
    }
    enable_interrupts();
    return 0;
}

/** @brief Defers execution of the invoking thread to a time determined by the
 *  scheduler.
 *
 *  If tid is -1 the scheduler will determine which thread to run next.
 *
 *  @param tid The tid of the thread to yield to.
 *  @return 0 on success, negative error code otherwise.
 */
int yield(int tid)
{
    if (tid == -1) {
        if (linklist_remove_head(&scheduler_queue, (void**)&tid) < 0) {
            //no threads so run idle
            assert (context_switch(idle_tid) == 0);
            return 0;
        }
    } else if (linklist_remove(&scheduler_queue, (void *)tid) < 0) {
        return -1;
    }

    linklist_add_tail(&scheduler_queue, (void*)tid);

    assert (context_switch(tid) == 0);

    //FIXME: not always from timer call
    notify_interrupt_complete(); //we are coming from timer call but not returning

    return 0;
}

// TODO Add atomicity with make runnable, use either a spinlock or disable interrupts


/** @brief Deschedules the calling thread.
 *
 *  If the integer pointed to by reject is non-zero, the call returns
 *  immediately returns with return value 0. If the integer pointed to by
 *  reject is zero, then the calling thread will not be run by the scheduler
 *  until a make_runnable() call is made specifying the descheduled thread, at
 *  point deschedule() will return zero.
 *
 *  @param flag If this points to a nonzero integer, deschedule returns
 *  immediately.
 *  @return 0 on success, negative error code otherwise.
 */
int deschedule(int *flag)
{
    return deschedule_kern(flag, true);
}

/** @brief Deschedules the calling thread.
 *
 *  If the integer pointed to by reject is non-zero, the call returns
 *  immediately returns with return value 0.  If the integer pointed to by
 *  reject is zero, then the calling thread will not be run by the scheduler
 *  until a make_runnable() call is made specifying the descheduled thread, at
 *  point deschedule() will return zero. The thread cannot be rescheduled
 *  by a call to make_runnable if user is false.
 *
 *  @param flag If this points to a nonzero integer, deschedule returns
 *  immediately.
 *  @param user User bool
 *  @return 0 on success, negative error code otherwise.
 */
int deschedule_kern(int *flag, bool user)
{
    if (flag == NULL)
        return -1;
    if (*flag) {
        return 0;
    }

    assert(linklist_remove(&scheduler_queue, (void*)gettid()) == 0);

    if (user) {
        hashtable_add(&descheduled_tids, gettid(), NULL);
    }

    if (yield(-1) < 0) {
        return -2;
    }

    return 0;
}

/** @brief Makes the descheduled thread with ID tid runnable if the thread
 *  was descheduled by a call to deschedule.
 *
 *  @param tid The tid of the thread to make runnable.
 *  @return 0 on success, negative error code otherwise.
 */
int make_runnable(int tid)
{
    return make_runnable_kern(tid, true);
}

/** @brief Makes the descheduled thread with ID tid runnable.
 *
 *  It is an error to try to reschedule a thread who was descheduled by
 *  deschedule_kern(tid, false) if user is true.
 *
 *  @param tid The tid of the thread to make runnable.
 *  @param user User bool
 *  @return 0 on success, negative error code otherwise.
 */
int make_runnable_kern(int tid, bool user)
{
    if (linklist_contains(&scheduler_queue, (void *)tid)) {
        return -1;
    }

    if (user) {
        if (hashtable_get(&descheduled_tids, tid, NULL) == 0)
            assert(hashtable_remove(&descheduled_tids, tid) == 0);
        else
            return -2;
    }

    linklist_add_tail(&scheduler_queue, (void*)tid);

    return 0;
}

/** @brief Deschedules the calling thread until atleast ticks timer interrupts
 *  have occured after the call.
 *
 *  Returns immediately if ticks is zero.  Returns an integer error code less
 *  than zero if ticks is negative.  Returns zero otherwise.
 *
 *  @param ticks The number of timer interrupts the thread is descheduled.
 *  @return 0 on success, negative error code otherwise.
 */
int sleep(int ticks)
{
    if (ticks < 0) {
        return -1;
    }

    if (ticks == 0) {
        return 0;
    }

    sleep_info_t sleep_info;
    sleep_info.tid = gettid();
    sleep_info.wake_ticks = get_ticks() + ticks;

    linklist_add_sorted(&sleep_queue, (void *)&sleep_info, sleep_info_cmp);

    int flag = 0;
    if (deschedule(&flag) < 0) {
        return -2;
    }

    return 0;
}

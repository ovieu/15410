/** @file mutex.c
 *  @brief This file implements mutexes.
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */

#include <mutex.h>
#include <atom_xchg.h>
#include <stdlib.h>
#include <syscall.h>

/** @brief Initializes a mutex.
 *
 *  @param mp The mutex.
 *  @return 0 on success, negative error code otherwise.
 */
int mutex_init(mutex_t *mp) {
    if (mp == NULL) {
        return -1;
    }

    mp->lock = 0;
    mp->tid = -1;
    mp->count = 0;
    mp->count_lock = 0;

    mp->valid = 1;

    return 0;
}

/** @brief "Deactivates" a mutex.
 *
 *  @param mp The mutex.
 *  @return Void.
 */
void mutex_destroy(mutex_t *mp) {
    if (mp == NULL || !mp->valid) {
        return;
    }

    mp->valid = 0;

    while (mp->count > 0) {
        yield(mp->tid);
    }
}

/** @brief Locks a mutex.
 *
 *  @param mp The mutex.
 *  @return Void.
 */
void mutex_lock(mutex_t *mp) {
    if (mp == NULL || !mp->valid) {
        return;
    }

    while (atom_xchg(&mp->count_lock, 1) != 0) {
        if (!mp->valid) {
            return;
        }
        yield(-1);
    }
    mp->count++;
    mp->count_lock = 0;

    while (atom_xchg(&mp->lock, 1) != 0) {
        yield(mp->tid);
    }

    mp->tid = gettid();
}

/** @brief Unlocks a mutex.
 *
 *  @param mp The mutex.
 *  @return Void.
 */
void mutex_unlock(mutex_t *mp) {
    if (mp == NULL || !mp->valid || !mp->lock ||
        mp->tid != gettid()) {
        return;
    }

    while (atom_xchg(&mp->count_lock, 1) != 0) {
        yield(-1);
    }
    mp->count--;
    mp->count_lock = 0;

    mp->tid = -1;
    mp->lock = 0;
}

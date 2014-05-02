/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * \file
 * Thread utilities functions.
 */

#ifndef _HAKA_THREAD_H
#define _HAKA_THREAD_H

#include <stddef.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include <haka/types.h>
#include <haka/compiler.h>


/**
 * \defgroup Thread Thread
 * Thread functions
 * @{
 */

/**
 * Get the number of thread to be used by the application for
 * packet capture.
 */
int      thread_get_packet_capture_cpu_count();

/**
 * Set the number of thread to be used by the application for
 * packet capture.
 * @param count Number of thread to use for capture.
 */
void     thread_set_packet_capture_cpu_count(int count);

/**
 * Get the number of CPUs.
 */
int      thread_get_cpu_count();

typedef pthread_t thread_t; /**< Opaque thread type. */

#define THREAD_CANCELED  PTHREAD_CANCELED /**< Thread return value when canceled. */

/**
 * Start a new thread.
 */
bool     thread_create(thread_t *thread, void *(*main)(void*), void *param);

/**
 * Join a thread.
 */
bool     thread_join(thread_t thread, void **ret);

/**
 * Cancel a running thread.
 */
bool     thread_cancel(thread_t thread);

/**
 * Get current thread id.
 */
int      thread_getid();

/**
 * Set current thread id.
 * @param id New thread identifier.
 */
void     thread_setid(int id);

/**
 * Set signal mask on a thread.
 */
bool     thread_sigmask(int how, sigset_t *set, sigset_t *oldset);

/**
 * Thread cancel mode.
 */
enum thread_cancel_t {
	THREAD_CANCEL_DEFERRED,    /**< Deferred mode. */
	THREAD_CANCEL_ASYNCHRONOUS /**< Asynchronous mode. */
};

/**
 * Change thread support for cancel.
 */
bool     thread_setcancelstate(bool enable);

/**
 * Change thread cancel mode.
 */
bool     thread_setcanceltype(enum thread_cancel_t type);

/**
 * Check for cancel.
 */
void     thread_testcancel();

/**
 * Run a function in protected mode. If a cancel is raised, then a cleanup
 * function is called before leaving the thread.
 */
void     thread_protect(void (*run)(void *), void *runarg, void (*finish)(void *), void *finisharg);

/**
 * Get the main thread handle.
 */
thread_t  thread_main();

/**
 * Get the current thread.
 */
thread_t  thread_self();

/**
 * Check if two threads are the same.
 */
bool      thread_equal(thread_t a, thread_t b);

/**
 * Raise a signal on the given thread.
 */
bool      thread_kill(thread_t thread, int sig);


/**@}*/

/**
 * \defgroup Mutex Mutex
 * @{
 */

typedef pthread_mutex_t mutex_t; /**< Opaque mutex type. */

#define MUTEX_INIT PTHREAD_MUTEX_INITIALIZER /**< Mutex static initializer. */

bool mutex_init(mutex_t *mutex, bool recursive);
bool mutex_destroy(mutex_t *mutex);
bool mutex_lock(mutex_t *mutex);
bool mutex_trylock(mutex_t *mutex);
bool mutex_unlock(mutex_t *mutex);

/**@}*/

/**
 * \defgroup Spinlock Spinlock
 * @{
 */

typedef pthread_spinlock_t spinlock_t; /**< Opaque spinlock type. */

bool spinlock_init(spinlock_t *lock);
bool spinlock_destroy(spinlock_t *lock);
bool spinlock_lock(spinlock_t *lock);
bool spinlock_trylock(spinlock_t *lock);
bool spinlock_unlock(spinlock_t *lock);

/**@}*/

/**
 * \defgroup RWLock Read-Write Locks
 * @{
 */

typedef pthread_rwlock_t rwlock_t; /**< Opaque read-write lock type. */

#define RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER /**< Read-write lock static initializer. */

bool rwlock_init(rwlock_t *rwlock);
bool rwlock_destroy(rwlock_t *rwlock);
bool rwlock_readlock(rwlock_t *rwlock);
bool rwlock_tryreadlock(rwlock_t *rwlock);
bool rwlock_writelock(rwlock_t *rwlock);
bool rwlock_trywritelock(rwlock_t *rwlock);
bool rwlock_unlock(rwlock_t *rwlock);

/**@}*/

/**
 * \defgroup Semaphore Semaphore
 * @{
 */

typedef sem_t semaphore_t; /**< Opaque semaphore type. */

bool semaphore_init(semaphore_t *semaphore, uint32 initial);
bool semaphore_destroy(semaphore_t *semaphore);
bool semaphore_wait(semaphore_t *semaphore);
bool semaphore_post(semaphore_t *semaphore);

/**@}*/

/**
 * \defgroup Barrier Barrier
 * @{
 */

typedef pthread_barrier_t barrier_t; /**< Opaque barrier type. */

bool barrier_init(barrier_t *barrier, uint32 count);
bool barrier_destroy(barrier_t *barrier);
bool barrier_wait(barrier_t *barrier);

/**@}*/

/**
 * \defgroup TLS Thread local storage
 * @{
 */

typedef pthread_key_t local_storage_t; /**< Opaque local storage type. */

/**
 * Initialize thread local storage. The parameter `destructor` allows to set a cleanup
 * function to call when the storage is destroyed.
 *
 * \returns True on success. Use clear_error() to get details about the error.
 */
bool local_storage_init(local_storage_t *key, void (*destructor)(void *));

/**
 * Destroy a thread local storage.
 *
 * \returns True on success. Use clear_error() to get details about the error.
 */
bool local_storage_destroy(local_storage_t *key);

/**
 * Get the value of the thread local storage.
 *
 * \returns The value associated with the current thread.
 */
void *local_storage_get(local_storage_t *key);

/**
 * Set the value of the thread local storage.
 *
 * \returns True on success. Use clear_error() to get details about the error.
 */
bool local_storage_set(local_storage_t *key, const void *value);

/**@}*/

/**
 * \defgroup Atomic Atomic counter
 * @{
 */

/**
 * 32 bit atomic opaque type.
 */
typedef volatile uint32 atomic_t;

/**
 * Increment an atomic counter.
 *
 * \return The new value after increment.
 */
INLINE uint32 atomic_inc(atomic_t *v) { return __sync_add_and_fetch(v, 1); }

/**
 * Decrement an atomic counter.
 *
 * \return The new value after decrement.
 */
INLINE uint32 atomic_dec(atomic_t *v) { return __sync_sub_and_fetch(v, 1); }

/**
 * Get the value of an atomic counter.
 */
INLINE uint32 atomic_get(atomic_t *v) { return *v; }

/**
 * Set the current value of an atomic counter.
 */
INLINE void atomic_set(atomic_t *v, uint32 x) { *v = x; }


/* Atomic counter (64 bits) */

#if defined(__x86_64__) || defined(__doxygen__)

/**
 * 64 bit atomic opaque type.
 */
typedef volatile uint64 atomic64_t;

/**
 * Set the current value of a 64 bit atomic counter.
 */
INLINE void atomic64_set(atomic64_t *v, uint64 x) { *v = x; }

/**
 * Initialize a 64 bit atomic counter.
 */
INLINE void atomic64_init(atomic64_t *v, uint64 x) { atomic64_set(v, x); }

/**
 * Destroy a 64 bit atomic counter.
 */
INLINE void atomic64_destroy(atomic64_t *v) { }

/**
 * Increment a 64 bit atomic counter.
 *
 * \return The new value after increment.
 */
INLINE uint64 atomic64_inc(atomic64_t *v) { return __sync_add_and_fetch(v, 1); }

/**
 * Decrement a 64 bit atomic counter.
 *
 * \return The new value after decrement.
 */
INLINE uint64 atomic64_dec(atomic64_t *v) { return __sync_sub_and_fetch(v, 1); }

/**
 * Get the value of a 64 bit atomic counter.
 */
INLINE uint64 atomic64_get(atomic64_t *v) { return *v; }

#else

typedef struct  {
	uint64      value;
	spinlock_t  spinlock;
} atomic64_t;

void atomic64_init(atomic64_t *v, uint64 x);
void atomic64_destroy(atomic64_t *v);
uint64 atomic64_inc(atomic64_t *v);
uint64 atomic64_dec(atomic64_t *v);
INLINE uint64 atomic64_get(atomic64_t *v) { return v->value; }
void atomic64_set(atomic64_t *v, uint64 x);

#endif

/**@}*/

#endif /* _HAKA_THREAD_H */

/**
 * @file haka/thread.h
 * @brief Threads utilities
 * @author Pierre-Sylvain Desse
 *
 * The file contains the thread API functions.
 */

/**
 * @defgroup Multithread Multi-thread
 * @brief Multi threading API functions and structures.
 * @ingroup API
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
 * Gets the number of thread to be used by the application for
 * packet capture.
 * @ingroup Multithread
 */
int thread_get_packet_capture_cpu_count();


/**
 * Gets the number of CPUs.
 * @ingroup Multithread
 */
int thread_get_cpu_count();


/**
 * Gets current thread id.
 * @ingroup Multithread
 */
int thread_get_id();

void thread_set_id(int id);


/**
 * Atomic counter. Opaque object.
 * @ingroup Multithread
 */
typedef volatile uint32 atomic_t;

/**
 * Increment a counter and return its value atomically.
 * @param v The atomic counter.
 * @return The value after adding 1 to it.
 * @ingroup Multithread
 */
INLINE uint32 atomic_inc(atomic_t *v)
{
	return __sync_add_and_fetch(v, 1);
}

/**
 * Decrement a counter and return its value atomically.
 * @param v The atomic counter.
 * @return The value after subtracting 1 to it.
 * @ingroup Multithread
 */
INLINE uint32 atomic_dec(atomic_t *v)
{
	return __sync_sub_and_fetch(v, 1);
}

/**
 * Gets the current value of the atomic counter.
 * @param v The counter value.
 * @return The counter value.
 * @ingroup Multithread
 */
INLINE uint32 atomic_get(atomic_t *v)
{
	return *v;
}

/**
 * Sets the current value of the atomic counter.
 * @param v The counter value.
 * @param x The value to set.
 * @ingroup Multithread
 */
INLINE void atomic_set(atomic_t *v, uint32 x)
{
	*v = x;
}


/**
 * Mutex. Opaque object.
 * @ingroup Multithread
 */
typedef pthread_mutex_t mutex_t;

/**
 * Static mutex initializer.
 * @ingroup Multithread
 */
#define MUTEX_INIT PTHREAD_MUTEX_INITIALIZER

/**
 * Initializes a mutex.
 * @param mutex The mutex to initialize.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool mutex_init(mutex_t *mutex);

/**
 * Destroy a mutex.
 * @param mutex The mutex to destroy.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool mutex_destroy(mutex_t *mutex);

/**
 * Locks a mutex.
 * @param mutex The mutex to lock.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool mutex_lock(mutex_t *mutex);

/**
 * Try to lock a mutex.
 * @param mutex The mutex to lock.
 * @return True if successful, False if the lock could not be taken.
 * Use check_error() and clear_error() to get details about the error.
 * @ingroup Multithread
 */
bool mutex_trylock(mutex_t *mutex);

/**
 * Unlocks a mutex.
 * @param mutex The mutex to unlock.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool mutex_unlock(mutex_t *mutex);


/**
 * Semaphore. Opaque object.
 * @ingroup Multithread
 */
typedef sem_t semaphore_t;

/**
 * Initialize a new semaphore.
 * @param semaphore Semaphore to initialize.
 * @param initial Initial semaphore value.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool semaphore_init(semaphore_t *semaphore, uint32 initial);

/**
 * Destroy a semaphore.
 * @param semaphore Semaphore to destroy.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool semaphore_destroy(semaphore_t *semaphore);

/**
 * Wait on a semaphore.
 * @param semaphore Semaphore.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool semaphore_wait(semaphore_t *semaphore);

/**
 * Post on a semaphore.
 * @param semaphore Semaphore.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool semaphore_post(semaphore_t *semaphore);


/**
 * Thread local storage. Opaque object.
 * @ingroup Multithread
 */
typedef pthread_key_t local_storage_t;

/**
 * Initialize thread local storage.
 * @param key The thread local storage object.
 * @param destructor The destructor called when the storage is destroyed.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool local_storage_init(local_storage_t *key, void (*destructor)(void *));

/**
 * Destroy a thread local storage.
 * @param key The thread local storage object.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool local_storage_destroy(local_storage_t *key);

/**
 * Gets the value of the thread local storage.
 * @param key The thread local storage object.
 * @return The value associated with the current thread.
 * @ingroup Multithread
 */
void *local_storage_get(local_storage_t *key);

/**
 * Sets the value of the thread local storage.
 * @param key The thread local storage object.
 * @param value The value associated with the current thread.
 * @return true on success. Use clear_error to get details about the error.
 * @ingroup Multithread
 */
bool local_storage_set(local_storage_t *key, const void *value);


#endif /* _HAKA_THREAD_H */

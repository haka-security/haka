
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <haka/thread.h>
#include <haka/error.h>


int thread_get_packet_capture_cpu_count()
{
	return thread_get_cpu_count();
}

int thread_get_cpu_count()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

static local_storage_t thread_id_key;

INIT static void thread_id_init()
{
	assert(local_storage_init(&thread_id_key, NULL));
}

int thread_get_id()
{
	return (ptrdiff_t)local_storage_get(&thread_id_key);
}

void thread_set_id(int id)
{
	local_storage_set(&thread_id_key, (void*)(ptrdiff_t)id);
}


/*
 * Mutex
 */

bool mutex_init(mutex_t *mutex, bool recursive)
{
	int err;
	pthread_mutexattr_t attr;

	err = pthread_mutexattr_init(&attr);
	if (err) {
		error(L"mutex error: %s", errno_error(err));
		return false;
	}

	if (recursive)
		err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	else
		err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

	if (err) {
		error(L"mutex error: %s", errno_error(err));
		return false;
	}

	err = pthread_mutex_init(mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	if (err) {
		error(L"mutex error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool mutex_destroy(mutex_t *mutex)
{
	const int err = pthread_mutex_destroy(mutex);
	if (err) {
		error(L"mutex error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool mutex_lock(mutex_t *mutex)
{
	const int err = pthread_mutex_lock(mutex);
	if (err) {
		error(L"mutex error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool mutex_trylock(mutex_t *mutex)
{
	const int err = pthread_mutex_trylock(mutex);
	if (err == 0) return true;
	else if (err == EBUSY) return false;
	else {
		error(L"mutex error: %s", errno_error(err));
		return false;
	}
}

bool mutex_unlock(mutex_t *mutex)
{
	const int err = pthread_mutex_unlock(mutex);
	if (err) {
		error(L"mutex error: %s", errno_error(err));
		return false;
	}
	return true;
}


/*
 * Semaphore
 */

bool semaphore_init(semaphore_t *semaphore, uint32 initial)
{
	const int err = sem_init(semaphore, 0, initial);
	if (err) {
		error(L"semaphore error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool semaphore_destroy(semaphore_t *semaphore)
{
	const int err = sem_destroy(semaphore);
	if (err) {
		error(L"semaphore error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool semaphore_wait(semaphore_t *semaphore)
{
	const int err = sem_wait(semaphore);
	if (err) {
		error(L"semaphore error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool semaphore_post(semaphore_t *semaphore)
{
	const int err = sem_post(semaphore);
	if (err) {
		error(L"semaphore error: %s", errno_error(err));
		return false;
	}
	return true;
}


/*
 * Thread local storage.
 */

bool local_storage_init(local_storage_t *key, void (*destructor)(void *))
{
	const int err = pthread_key_create(key, destructor);
	if (err) {
		error(L"local storage error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool local_storage_destroy(local_storage_t *key)
{
	const int err = pthread_key_delete(*key);
	if (err) {
		error(L"local storage error: %s", errno_error(err));
		return false;
	}
	return true;
}

void *local_storage_get(local_storage_t *key)
{
	return pthread_getspecific(*key);
}

bool local_storage_set(local_storage_t *key, const void *value)
{
	const int err = pthread_setspecific(*key, value);
	if (err) {
		error(L"local storage error: %s", errno_error(err));
		return false;
	}
	return true;
}

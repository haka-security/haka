
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <haka/thread.h>
#include <haka/error.h>


static int thread_capture_count = 0;

int thread_get_packet_capture_cpu_count()
{
	if (thread_capture_count == 0)
		return thread_get_cpu_count();
	else
		return thread_capture_count;
}

void thread_set_packet_capture_cpu_count(int count)
{
	thread_capture_count = count;
}

int thread_get_cpu_count()
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

static local_storage_t thread_id_key;

INIT static void thread_id_init()
{
	UNUSED const bool ret = local_storage_init(&thread_id_key, NULL);
	assert(ret);
}

int thread_getid()
{
	return (ptrdiff_t)local_storage_get(&thread_id_key);
}

void thread_setid(int id)
{
	local_storage_set(&thread_id_key, (void*)(ptrdiff_t)id);
}

bool thread_create(thread_t *thread, void *(*main)(void*), void *param)
{
	const int err = pthread_create(thread, NULL, main, param);
	if (err) {
		error(L"thread creation error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool thread_join(thread_t thread, void **ret)
{
	const int err = pthread_join(thread, ret);
	if (err) {
		error(L"thread join error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool thread_cancel(thread_t thread)
{
	const int err = pthread_cancel(thread);
	if (err) {
		error(L"thread cancel error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool thread_sigmask(int how, sigset_t *set, sigset_t *oldset)
{
	const int err = pthread_sigmask(how, set, oldset);
	if (err) {
		error(L"thread sigmask error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool thread_setcanceltype(enum thread_cancel_t type)
{
	int err;

	switch (type) {
	case THREAD_CANCEL_DEFERRED: err = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL); break;
	case THREAD_CANCEL_ASYNCHRONOUS: err = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); break;
	default: error(L"invalid thread cancel mode"); return false;
	}

	if (err) {
		error(L"thread cancel type error: %s", errno_error(err));
		return false;
	}

	return true;
}

bool thread_setcancelstate(bool enable)
{
	const int err = pthread_setcancelstate(enable ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE, NULL);
	if (err) {
		error(L"thread set cancel state error: %s", errno_error(err));
		return false;
	}
	return true;
}

void thread_testcancel()
{
	pthread_testcancel();
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
 * Read-Write Locks
 */

bool rwlock_init(rwlock_t *rwlock)
{
	const int err = pthread_rwlock_init(rwlock, NULL);
	if (err) {
		error(L"rwlock error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool rwlock_destroy(rwlock_t *rwlock)
{
	const int err = pthread_rwlock_destroy(rwlock);
	if (err) {
		error(L"rwlock error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool rwlock_readlock(rwlock_t *rwlock)
{
	const int err = pthread_rwlock_rdlock(rwlock);
	if (err) {
		error(L"rwlock error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool rwlock_tryreadlock(rwlock_t *rwlock)
{
	const int err = pthread_rwlock_tryrdlock(rwlock);
	if (err == 0) return true;
	else if (err == EBUSY) return false;
	else {
		error(L"rwlock error: %s", errno_error(err));
		return false;
	}
}

bool rwlock_writelock(rwlock_t *rwlock)
{
	const int err = pthread_rwlock_wrlock(rwlock);
	if (err) {
		error(L"rwlock error: %s", errno_error(err));
		return false;
	}
	return true;
}

bool rwlock_trywritelock(rwlock_t *rwlock)
{
	const int err = pthread_rwlock_trywrlock(rwlock);
	if (err == 0) return true;
	else if (err == EBUSY) return false;
	else {
		error(L"rwlock error: %s", errno_error(err));
		return false;
	}
}

bool rwlock_unlock(rwlock_t *rwlock)
{
	const int err = pthread_rwlock_unlock(rwlock);
	if (err) {
		error(L"rwlock error: %s", errno_error(err));
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

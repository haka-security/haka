.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: c

Threads
=======

.. c:function:: int thread_get_packet_capture_cpu_count()

    Get the number of thread to be used by the application for
    packet capture.

.. c:function:: int thread_get_cpu_count()

    Get the number of CPUs.

.. c:function:: int thread_get_id()

    Get current thread id.

Atomic
------

.. c:type:: atomic_t

    32 bit atomic integer.

.. c:function:: uint32 atomic_inc(atomic_t *v)

    Increment a counter `v` and return its new value atomically.

.. c:function:: uint32 atomic_dec(atomic_t *v)

    Decrement a counter `v` and return its new value atomically.

.. c:function:: uint32 atomic_get(atomic_t *v)

    Get the current value of the atomic counter `v`.

.. c:function:: void atomic_set(atomic_t *v, uint32 x)

    Set the current value of the atomic counter `v` to `x`.

Mutex
-----

.. c:type:: mutex_t

    Mutex opaque type.

.. c:var:: MUTEX_INIT

    Initialize static mutex.

.. c:function:: bool mutex_init(mutex_t *mutex, bool recursive)

    Initialize a mutex `mutex`. With `recursive`, the mutex can be created
    recursive (ie. can be re-entered by the same thread).

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_destroy(mutex_t *mutex)

    Destroy a mutex.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_lock(mutex_t *mutex)

    Lock a mutex.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_trylock(mutex_t *mutex)

    Try to lock a mutex.

    :returns: True if successful, False if the lock could not be taken. Use :c:func:`check_error`
        and :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_unlock(mutex_t *mutex)

    Unlock a mutex.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

Semaphore
---------

.. c:type:: semaphore_t

    Semaphore opaque type.

.. c:function:: bool semaphore_init(semaphore_t *semaphore, uint32 initial)

    Initialize a new semaphore with initial value of `initial`.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool semaphore_destroy(semaphore_t *semaphore)

    Destroy a semaphore.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool semaphore_wait(semaphore_t *semaphore)

    Wait on a semaphore.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool semaphore_post(semaphore_t *semaphore)

    Post on a semaphore.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

Thread local storage
--------------------

.. c:type:: local_storage_t

    Thread local storage opaque type.

.. c:function:: bool local_storage_init(local_storage_t *key, void (*destructor)(void *))

    Initialize thread local storage. The parameter `destructor` allows to set a cleanup
    function to call when the storage is destroyed.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool local_storage_destroy(local_storage_t *key)

    Destroy a thread local storage.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: void *local_storage_get(local_storage_t *key)

    Get the value of the thread local storage.

    :returns: The value associated with the current thread.

.. c:function:: bool local_storage_set(local_storage_t *key, const void *value)

    Set the value of the thread local storage.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

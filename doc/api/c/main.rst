
.. highlightlang:: c

C API
=====

This document describes the modules of the API.

Types
-----

.. c:type:: wchar_t

    Wide chars

.. c:type:: bool

    Boolean. Its value can be :c:data:`true` or  :c:data:`false`.

.. c:type:: int8
            int16
            int32
            int64
            uint8
            uint16
            uint32
            uint64

    Integer basic type.

.. c:macro:: SWAP(type, x)
             SWAP_TO_BE(type, x)
             SWAP_FROM_BE(type, x)
             SWAP_TO_LE(type, x)
             SWAP_FROM_LE(type, x)

    Byte swapping utility macro. The `type` should be the c type of the value `x`
    (ie. :c:type:`int32`, :c:type:`uint64`...)

.. c:macro:: GET_BIT(v, i)
             SET_BIT(v, i, x)

    Gets and sets the bit `i` from an integer `v`.

.. c:macro:: GET_BITS(v, i, j)
             SET_BITS(v, i, j, x)

    Gets and sets the bits in range [`i` ; `j`] from an integer `v`.

Utilities
---------

Errors
^^^^^^

.. c:function:: void error(const wchar_t *error, ...)

    Set an error message to be reported to the callers. The function follows the printf API
    and can be used in multi-threaded application. The error can then be retrieved using
    :c:func:`clear_error` and :c:func:`check_error`.

.. c:function:: const char *errno_error(int err)

    Convert the errno value to a human readable error message.

    .. note:: The returned string will be erased by the next call to this function. The function
        thread-safe and can be used in multi-threaded application.

.. c:function:: bool check_error()

    Checks if an error has occurred. This function does not clear error flag.

.. c:function:: const wchar_t *clear_error()

    Gets the error message and clear the error state.


Threads
-------

.. c:function:: int thread_get_packet_capture_cpu_count()

    Gets the number of thread to be used by the application for
    packet capture.

.. c:function:: int thread_get_cpu_count()

    Gets the number of CPUs.

.. c:function:: int thread_get_id()

    Gets current thread id.

Atomic :c:type:`atomic_t`
^^^^^^^^^^^^^^^^^^^^^^^^^

.. c:type:: atomic_t

    32 bit atomic integer.

.. c:function:: uint32 atomic_inc(atomic_t *v)

    Increment a counter `v` and return its new value atomically.

.. c:function:: uint32 atomic_dec(atomic_t *v)

    Decrement a counter `v` and return its new value atomically.

.. c:function:: uint32 atomic_get(atomic_t *v)

    Gets the current value of the atomic counter `v`.

.. c:function:: void atomic_set(atomic_t *v, uint32 x)

    Sets the current value of the atomic counter `v` to `x`.

Mutex :c:type:`mutex_t`
^^^^^^^^^^^^^^^^^^^^^^^

.. c:type:: mutex_t

    Mutex opaque type.

.. c:var:: MUTEX_INIT

    Initialize static mutex.

.. c:function:: bool mutex_init(mutex_t *mutex, bool recursive)

    Initializes a mutex `mutex`. With `recursive`, the mutex can be created
    recursive (ie. can be re-entered by the same thread).

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_destroy(mutex_t *mutex)

    Destroy a mutex.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_lock(mutex_t *mutex)

    Locks a mutex.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_trylock(mutex_t *mutex)

    Try to lock a mutex.

    :returns: True if successful, False if the lock could not be taken. Use :c:func:`check_error`
        and :c:func:`clear_error` to get details about the error.

.. c:function:: bool mutex_unlock(mutex_t *mutex)

    Unlocks a mutex.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

Semaphore :c:type:`semaphore_t`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

Thread local storage :c:type:`local_storage_t`
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

    Gets the value of the thread local storage.

    :returns: The value associated with the current thread.

.. c:function:: bool local_storage_set(local_storage_t *key, const void *value)

    Sets the value of the thread local storage.

    :returns: True on success. Use :c:func:`clear_error` to get details about the error.

Log
---

.. c:type:: log_level

    Logging level constants.

    .. c:var:: HAKA_LOG_FATAL
               HAKA_LOG_ERROR
               HAKA_LOG_WARNING
               HAKA_LOG_INFO
               HAKA_LOG_DEBUG

.. c:function:: const char *level_to_str(log_level level)

    Convert a logging level to a human readable string.

    :returns: A string representing the logging level. This string is constant and should
        not be freed.

.. c:function:: void message(log_level level, const wchar_t *module, const wchar_t *message)

    Log a message without string formating.

.. c:function:: void messagef(log_level level, const wchar_t *module, const wchar_t *fmt, ...)

    Log a message with string formating.

.. c:function:: void setlevel(log_level level, const wchar_t *module)

    Sets the logging level to display for a given module name. The `module` parameter can be
    `NULL` in which case it will set the default level.

.. c:function:: log_level getlevel(const wchar_t *module)

    Gets the logging level for a given module name.


Packet
------

.. c:type:: struct packet

    Opaque packet structure.

.. c:type:: filter_result

    .. c:var:: FILTER_ACCEPT
               FILTER_DROP

.. c:function:: size_t packet_length(struct packet *pkt)

    Gets the length of a packet.

.. c:function:: const uint8 *packet_data(struct packet *pkt)

    Gets the data of a packet.

.. c:function:: const char *packet_dissector(struct packet *pkt)

    Gets packet dissector to use.

.. c:function:: uint8 *packet_data_modifiable(struct packet *pkt)

    Make a packet modifiable.

.. c:function:: int packet_resize(struct packet *pkt, size_t size)

    Resize a packet.

.. c:function:: void packet_drop(struct packet *pkt)

    Drop a packet. The packet will be released and should not be used anymore
    after this call.

.. c:function:: void packet_accept(struct packet *pkt);

    Accept a packet. The packet will be released and should not be used anymore
    after this call.

.. c:function:: int packet_receive(struct packet_module_state *state, struct packet **pkt)

    Receive a packet from the capture module. This function will wait if no packet is
    available.


Modules
-------

.. c:type:: struct module

    Module structure.

    .. c:type:: type

        Module type.

        .. c:var:: MODULE_UNKNOWN
                   MODULE_PACKET
                   MODULE_LOG
                   MODULE_EXTENSION

    .. c:var:: const wchar_t *name
    .. c:var:: const wchar_t *description
    .. c:var:: const wchar_t *author

    .. c:function:: int (*init)(int argc, char *argv[])

        Initialize the module. This function is called by the application.

        :returns: Non zero in case of an error. In this case the module will be
            unloaded but cleanup is not going to be called.

    .. c:function:: void (*cleanup)()

        Cleanup the module. This function is called by the application when the
        module is unloaded.

.. c:type:: struct log_module

    Logging module structure.

    .. c:var:: struct module module

    .. c:function:: int (*message)(log_level level, const wchar_t *module, const wchar_t *message)

        Messaging function called by the application.

        :returns: Non-zero in case of error.


.. c:type:: struct packet_module_state

    Opaque state structure.


.. c:type:: struct packet_module

    Packet module used to interact with the low-level packets. The module
    will be used to receive packets and set a verdict on them. It also define
    an interface to access the packet fields.

    .. c:var:: struct module module

    .. c:function:: bool (*multi_threaded)()

        Does this module supports multi-threading.

    .. c:function:: struct packet_module_state *(*init_state)(int thread_id)

        Initialize the packet module state. This function will be called to create
        multiple states if the module supports multi-threading.

    .. c:function:: void (*cleanup_state)(struct packet_module_state *state)

        Cleanup the packet module state.

    .. c:function:: int (*receive)(struct packet_module_state *state, struct packet **pkt)

        Callback used to receive a new packet. This function should block until
        a packet is received.

        :returns:Non zero in case of error.

    .. c:function:: void (*verdict)(struct packet *pkt, filter_result result)

        Apply a verdict on a received packet. The module should then apply this
        verdict on the underlying packet.

        :param pkt: The received packet. After calling this funciton the packet address
            is never used again by the application allow the module to free it if needed.
        :param result: The verdict to apply to this packet.

    .. c:function:: size_t (*get_length)(struct packet *pkt)

        Get the length of a packet.

    .. c:function:: uint8 *(*make_modifiable)(struct packet *pkt)

        Make the packet modifiable.

    .. c:function:: int (*resize)(struct packet *pkt, size_t size)

        Resize the packet to a new size.

    .. c:function:: uint64 (*get_id)(struct packet *pkt)

        Gets the id fo the packet.

    .. c:function:: const uint8 *(*get_data)(struct packet *pkt)

        Get the data of a packet.

    .. c:function:: const char *(*get_dissector)(struct packet *pkt)

        Get the packet dissector.


.. c:function:: struct module *module_load(const char *module_name,...)

    Load a module given its name. It is not needed to call module_addref on the result
    as this is done before returning.

    :returns: The loaded module structure or NULL in case of an error.

.. c:function:: void module_addref(struct module *module)

    Keep the module. Must match with a call to module_release
    otherwise the module will not be able to be removed correctly
    when unused.

.. c:function:: void module_release(struct module *module)

    Release a module.

.. c:function:: void module_set_path(const char *path)

    Sets the path used to load haka modules. This path must be in the form:

    .. code-block:: bash

        path/to/modules/*;another/path/*

.. c:function:: const char *module_get_path()

    Gets the modules path.

External modules
----------------

.. toctree::
    :glob:
    :maxdepth: 1

    ../../../modules/*/doc/c*

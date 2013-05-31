
.. highlightlang:: c

Error API
=========

Types
-----

.. c:type:: wchar_t

    Wide char

Functions
---------

.. c:function:: void error2x(const wchar_t *error, ...)
                void error(const wchar_t *error, ...)

    Set an error message to be reported to the callers. The function follow the printf API.

.. c:function:: const char *errno_error(int err)

    Convert the errno value to a human readable error message.

    :param err: Error number
    :return: The converter error number to string

.. c:function:: bool check_error()

    Checks if an error has occurred. This function does not clear error flag.

.. c:function:: const wchar_t *clear_error()

    Gets the error message and clear the error state.

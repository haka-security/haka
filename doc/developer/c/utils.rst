.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: c

Types
=====

.. c:type:: wchar_t

    Wide chars.

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

    Byte swapping utility macro. The `type` should be the C type of the value `x`
    (ie. :c:type:`int32`, :c:type:`uint64`...).

.. c:macro:: GET_BIT(v, i)
             SET_BIT(v, i, x)

    Get and sets the bit `i` from an integer `v`.

.. c:macro:: GET_BITS(v, i, j)
             SET_BITS(v, i, j, x)

    Get and sets the bits in range [`i` ; `j`] from an integer `v`.

Utilities
=========

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

    Check if an error has occurred. This function does not clear error flag.

.. c:function:: const wchar_t *clear_error()

    Get the error message and clear the error state.

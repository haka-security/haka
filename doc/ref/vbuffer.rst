.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Buffers
=======

Buffer
------

.. lua:currentmodule:: haka

.. lua:class:: vbuffer

    .. lua:function:: vbuffer(size)

        Allocates a new buffer of given ``size``.

    .. lua:function:: vbuffer(string)

        Allocates a new buffer from a ``string``. The string will be copied
        into the allocated buffer.

    .. lua:method:: insert(offset, data)

        :param data: Buffer
        :paramtype data: :lua:class:`vbuffer`

        Inserts a buffer ``data`` at the specified ``offset``.

    .. lua:method:: append(data)

        :param data: Buffer
        :paramtype data: :lua:class:`vbuffer`

        Inserts a buffer ``data`` at the end of the current buffer.

    .. lua:method:: replace(data, offset = 0, length = ALL)

        :param data: Buffer
        :paramtype data: :lua:class:`vbuffer`

        Replaces part of this buffer by some new ``data``.

    .. lua:method:: erase(offset, length = ALL)

        Erases part of this buffer.

    .. lua:method:: select(offset = 0, length = ALL)

        :returns: Return the data and the reference to be used in :lua:func:`vbuffer:restore`
        :rtype: :lua:class:`vbuffer`

        Selects part of this buffer to create a view to this part.

    .. lua:method:: restore(data)

        :param data: Buffer
        :paramtype data: :lua:class:`vbuffer`

        Restore selected part of a buffer.

    .. lua:method:: sub(offset = 0, length = ALL)

        :returns: Return a buffer view
        :rtype: :lua:class:`vbuffer`

        Create a view of part of this buffer.

    .. lua:method:: asnumber(endian = 'big', offset = 0, length = ALL)

        :param endian: Endianness of data (``'big'`` or ``'little'``)

        Read the buffer as a number.

    .. lua:method:: setnumber(value, endian = 'big', offset = 0, length = ALL)

        Write a number to the buffer.

    .. lua:method:: asbits(offset, length, endian = 'big')

        Read some bits the buffer and convert it to a number.

    .. lua:method:: setbits(value, offset, length, endian = 'big')

        Write a number to some bits of the buffer.

    .. lua:method:: asstring(offset = 0, length = ALL)

        Read part of the buffer as a string.

    .. lua:method:: setstring(value, offset = 0, length = ALL)

        Replace part of the buffer with a given string.

    .. lua:method:: setfixedstring(value, offset = 0, length = ALL)

        Replace, in-place, part of the buffer with a given string.


Stream
------

.. lua:class:: vbuffer_stream

    .. lua:function:: vbuffer_stream()

        Creates a new buffer stream.

    .. lua:method:: push(data)

        :param data: Buffer
        :paramtype data: :lua:class:`vbuffer`

        Push some data to the stream.

    .. lua:method:: pop()

        :returns: Return a buffer
        :rtype: :lua:class:`vbuffer`

        Pop data from the stream.

    .. lua:data:: data

        All available data in the stream.

    .. lua:data:: current

        Current stream position.


Iterator
--------

.. lua:class:: vbuffer_iterator

    Iterator on a buffer.

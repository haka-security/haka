.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Buffers
=======

Virtual buffer allows to access memory data using scatter lists. This enable
easy and efficient modifications on memory data.

Buffer
------

.. lua:currentmodule:: haka

.. lua:class:: vbuffer

    The main buffer object holds a list of memory chunks.

    .. lua:function:: vbuffer(size, zero=true)

        :param size: Requested memory size
        :param zero: Zero the memory

        Allocate a new buffer.

    .. lua:function:: vbuffer(string)

        :param string: Source data

        Allocate a new buffer from a ``string``. The string will be copied
        into the allocated buffer.

    .. lua:method:: pos(offset)

        :param offset: Position offset, if nil the end position will be returned
        :returns: Return an iterator inside this buffer
        :rtype: :lua:class:`vbuffer_iterator`

        Return an iterator at the given offset in the buffer.

    .. lua:method:: sub(offset=0, size=nil)

        :param offset: Position offset
        :param size: Size of the requested sub buffer, if nil all available data will be returned
        :returns: Return a part of the buffer
        :rtype: :lua:class:`vbuffer_sub`

        Return a sub part of the buffer.

    .. lua:method:: append(data)

        :param data: Data to append
        :paramtype data: :lua:class:`vbuffer`

        Inserts a buffer ``data`` at the end of the current buffer.

    .. lua:method:: clone(copy=false)

        :param copy: Copy or share the memory data.
        :returns: Return the cloned buffer
        :rtype: :lua:class:`vbuffer`

        Clone the buffer and optionally copy or share the memory data.

    .. lua:data:: modified

        Hold the state of the buffer. It is ``true`` if the buffer has been modified.


Sub buffer
----------

.. lua:class:: vbuffer_sub

    Object used to represent a sub part of a buffer.

    .. lua:method:: pos(offset)

        :param offset: Position offset, if nil the end position will be returned
        :returns: Return an iterator inside this sub buffer
        :rtype: :lua:class:`vbuffer_iterator`

        Return an iterator at the given offset in the sub buffer.

    .. lua:method:: sub(offset=0, size=nil)

        :param offset: Position offset
        :param size: Size of the requested sub buffer, if nil all available data will be returned
        :returns: Return a part of the sub buffer
        :rtype: :lua:class:`vbuffer_sub`

        Return a sub part of the sub buffer.

    .. lua:method:: zero()

        Zero the sub buffer memory data.

    .. lua:method:: erase()

        Erase the sub buffer.

    .. lua:method:: replace(data)

        :param data: Buffer
        :paramtype data: :lua:class:`vbuffer`

        Replace the sub buffer data by the ``data``.

    .. lua:method:: isflat()

        Return ``true`` if the buffer is flat (ie. it is made of one one memory chunk).

    .. lua:method:: flatten()

        Replace the sub buffer by a flat buffer contaning only one memory chunk. The memory
        will be copied if needed.

    .. lua:method:: size()

        Compute the size of the sub buffer.

    .. lua:method:: check_size(size)

        :param size: Minimum buffer size to check for

        Check if the buffer size is at least ``size``.

    .. lua:method:: select()

        :returns: Return a reference iterator and the extracted buffer.
        :rtype: :lua:class:`vbuffer_iterator` and :lua:class:`vbuffer`

        Select the sub buffer. The content of it will be extracted from the
        buffer. To reinsert the data, you can use :lua:func:`vbuffer_iterator::restore()`
        with the reference iterator that is returned as the first value.

    .. lua:method:: asnumber(endian = 'big')

        :param endian: Endianness of data (``'big'`` or ``'little'``)

        Read the sub buffer and convert it as a number.

    .. lua:method:: setnumber(value, endian = 'big')

        :param value: Value to set
        :param endian: Endianness of data (``'big'`` or ``'little'``)

        Write a number to the buffer.

    .. lua:method:: asbits(offset, length, endian = 'big')

        :param offset: Bit positon offset
        :param length: Size in bits
        :param endian: Endianness of data (``'big'`` or ``'little'``)

        Read some bits the buffer and convert it to a number.

    .. lua:method:: setbits(offset, length, value, endian = 'big')

        :param offset: Bit positon offset
        :param length: Size in bits
        :param value: Value to set
        :param endian: Endianness of data (``'big'`` or ``'little'``)

        Write a number to some bits of the buffer.

    .. lua:method:: asstring()

        Read the sub buffer and convert it to a string.

    .. lua:method:: setstring(value)

        :param value: Value to set

        Replace the sub buffer by the given string.

    .. lua:method:: setfixedstring(value)

        :param value: Value to set

        Replace, in-place, the sub buffer by the given string.


Iterator
--------

.. lua:class:: vbuffer_iterator

    Iterator on a buffer.

    .. lua:method:: mark(readonly=false)

        :param readonly: State of the mark

        Create a mark in the buffer at the iterator position.

    .. lua:method:: unmark()

        Remove a mark in the buffer. The iterator must point to a previously
        created mark.

    .. lua:method:: advance(size)

        :param size: Amount of bytes to skip
        :returns: The real amount of bytes skipped. This value can be smaller than size if not enough data are available.

        Advance the iterator of the given ``size`` bytes.

    .. lua:method:: available()

        Return the amount of bytes available after the iterator position.

    .. lua:method:: check_available(size)

        :param size: Minimum available bytes to check for

        Check if the iterator has at least ``size`` bytes available.

    .. lua:method:: insert(data)

        :param data: Buffer to insert

        Insert some data at the iterator position.

    .. lua:method:: restore(data)

        :param data: Buffer to restore

        Restore data at the iterator position. This iterator must point to
        the reference returned by the function :lua:func:`vbuffer_sub::select()`.

    .. lua:method:: sub(size)

        :param size: Size of the requested sub buffer, if nil all available data will be returned
        :returns: Return a part of the sub buffer
        :rtype: :lua:class:`vbuffer_sub`

        Create a sub buffer from the iterator position.

    .. data:: isend

        ``true`` if the iterator is at the end of buffer and no more data can
        be available even later in case of a stream.


Stream
------

.. lua:class:: vbuffer_stream

    .. lua:function:: vbuffer_stream()

        Create a new buffer stream.

    .. lua:method:: push(data)

        :param data: Buffer data
        :paramtype data: :lua:class:`vbuffer`

        Push some data to the stream.

    .. lua:method:: finish()

        Mark the end of the stream. Any call to :lua:func:`vbuffer_stream:push()`
        will result to an error.

    .. lua:method:: pop()

        :returns: Extracted data from the stream
        :rtype: :lua:class:`vbuffer`

        Pop available data from the stream.

    .. lua:data:: data

        All available data in the stream (as :lua:class:`vbuffer`).

    .. lua:data:: current

        Current stream position (as :lua:class:`vbuffer_iterator`).

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Buffers
=======

Virtual buffer allows to access memory data. Using scatter lists, it enables
easy and efficient modifications on memory data.

Buffer
------

.. haka:module:: haka

.. haka:class:: vbuffer
    :module:

    The main buffer object.

    .. haka:function:: vbuffer_allocate(size, zero = true) -> vbuffer

        :param size: Requested memory size.
        :paramtype size: number
        :param zero: Zero the memory.
        :paramtype zero: boolean
        :return vbuffer: Created buffer.
        :rtype vbuffer: :haka:class:`vbuffer`

        Allocate a new buffer with a specific size.

    .. haka:function:: vbuffer_from(string) -> vbuffer

        :param string: Source data.
        :paramtype string: string
        :return vbuffer: Created buffer.
        :rtype vbuffer: :haka:class:`vbuffer`

        Allocate a new buffer from a Lua ``string``. The string will be copied
        into the allocated buffer.

    .. haka:operator:: vbuffer[index] -> byte
                       vbuffer[index] = byte

        :param index: Byte index in the data.
        :paramtype index: number

        Get or set a byte in the data.

    .. haka:operator:: #vbuffer -> count

        :return count: Number of bytes in the buffer.
        :rtype count: number

        Get the size of the buffer.

    .. haka:method:: vbuffer:pos(offset = 'begin') -> iter

        :param offset: Position offset as number, ``'begin'`` or ``'end'``.
        :paramtype offset: number or string
        :return iter: Iterator at the required position.
        :rtype iter: :haka:class:`vbuffer_iterator`

        Return an iterator at the given offset in the buffer.

    .. haka:method:: vbuffer:sub(offset = 0, size = 'all') -> sub

        :param offset: Position offset.
        :paramtype offset: number
        :param size: Size of the requested sub buffer or ``'all'``.
        :paramtype size: number or string
        :return sub: Create sub-buffer.
        :rtype sub: :haka:class:`vbuffer_sub`

        Return a sub-part of the buffer.

    .. haka:method:: vbuffer:append(data)

        :param data: Data to append to the buffer.
        :paramtype data: :haka:class:`vbuffer`

        Insert some data at the end of the current buffer.

    .. haka:method:: vbuffer:clone(copy=false) -> clone

        :param copy: If ``true`` copy the memory otheerwise it is shared.
        :paramtype copy: boolean
        :return clone: Cloned buffer.
        :rtype clone: :haka:class:`vbuffer`

        Clone the buffer and optionally copy its memory data.

    .. haka:attribute:: vbuffer.modified
        :readonly:

        :type: boolean 

        ``true`` if the buffer has been modified, ``false`` otherwise.


Sub buffer
----------

.. haka:class:: vbuffer_sub
    :module:

    Object used to represent part of a buffer.

    .. haka:function:: vbuffer_sub(begin, end) -> sub

        :param begin: Beginning position.
        :paramtype begin: :haka:class:`vbuffer_iterator`
        :param end: Ending position.
        :paramtype end: :haka:class:`vbuffer_iterator`
        :return sub: Created sub buffer.
        :rtype sub: :haka:class:`vbuffer_sub`

        Create a sub buffer for two iterator.
        
        .. note:: The two iterators must be built from the same buffer.

    .. haka:operator:: vbuffer_sub[index] -> byte
                       vbuffer_sub[index] = byte

        :param index: Byte index in the data.
        :paramtype index: number

        Get or set a byte in the data.

    .. haka:operator:: #vbuffer_sub -> count

        :return count: Number of bytes in the buffer.
        :rtype count: number

        Get the size of the sub-buffer.

    .. haka:method:: vbuffer_sub:pos(offset = 'begin') -> iter

        :param offset: Position offset as number, ``'begin'`` or ``'end'``.
        :paramtype offset: number or string
        :return iter: Iterator at the required position.
        :rtype iter: :haka:class:`vbuffer_iterator`

        Return an iterator at the given offset in the buffer.

    .. haka:method:: vbuffer_sub:sub(offset = 0, size = 'all') -> sub

        :param offset: Position offset.
        :paramtype offset: number
        :param size: Size of the requested sub buffer or ``'all'``.
        :paramtype size: number or string
        :return sub: Create sub-buffer.
        :rtype sub: :haka:class:`vbuffer_sub`

        Return a sub-part of the buffer.

    .. haka:method:: vbuffer_sub:zero()

        Zero the sub buffer memory data.

    .. haka:method:: vbuffer_sub:erase()

        Erase the sub buffer.

    .. haka:method:: vbuffer_sub:replace(data)

        :param data: Buffer.
        :paramtype data: :haka:class:`vbuffer`

        Replace the sub buffer by some new data.
        
        .. note:: Data will be removed from the given parameter making *data* empty after this call.

    .. haka:method::  vbuffer_sub:isflat() -> isflat

        :return isflat: ``true`` if the buffer is flat.
        :rtype isflat: boolean

        Check if the buffer is flat (ie. it is made of one one memory chunk).

    .. haka:method:: vbuffer_sub:flatten()

        Replace the sub buffer by a flat buffer containing only one memory chunk. The memory
        will be copied if needed.

    .. haka:method:: vbuffer_sub:size() -> size

        :return size: Size of the sub-buffer.
        :rtype size: number

        Compute the size of the sub buffer.

    .. haka:method:: vbuffer_sub:check_size(size) -> enough, size

        :param size: Minimum buffer size to check for.
        :paramtype size: number
        :return enough: ``true`` if the sub-buffer is larger or equal to *size*.
        :rtype enough: boolean
        :return size: If *enough* is ``false``, size of the sub-buffer.
        :rtype size: number

        Check if the buffer size is larger or equal to a given value.

    .. haka:method:: vbuffer_sub:select() -> iter, buffer

        :return iter: Reference iterator.
        :rtype iter: :haka:class:`vbuffer_iterator`
        :return buffer: Extracted buffer.
        :rtype buffer: :haka:class:`vbuffer`

        Select this sub buffer. The content is extracted from the buffer.
        To reinsert the data, you can use :haka:func:`<vbuffer_iterator>.restore()`
        with the reference iterator that is returned as the first value.

    .. haka:method:: vbuffer_sub:asnumber(endian = 'big') -> num

        :param endian: Endianness of data (``'big'`` or ``'little'``)
        :paramtype endian: string
        :return num: Computed value.
        :rtype num: number

        Read the sub buffer and convert it to a number.

    .. haka:method:: vbuffer_sub:setnumber(value, endian = 'big')

        :param value: New value.
        :paramtype value: number
        :param endian: Endianness of data (``'big'`` or ``'little'``)
        :paramtype endian: string

        Write a number to the buffer.

    .. haka:method:: vbuffer_sub:asbits(offset, length, endian = 'big')

        :param offset: Bit positon offset.
        :paramtype offset: number
        :param length: Size in bits.
        :paramtype length: number
        :param endian: Endianness of data (``'big'`` or ``'little'``)
        :paramtype endian: string

        Read some bits the buffer and convert it to a number.

    .. haka:method:: vbuffer_sub:setbits(offset, length, value, endian = 'big')

        :param offset: Bit positon offset.
        :paramtype offset: number
        :param length: Size in bits.
        :paramtype length: number
        :param value: New value.
        :paramtype value: number
        :param endian: Endianness of data (``'big'`` or ``'little'``)
        :paramtype endian: string

        Write a number to some bits of the buffer.

    .. haka:method:: vbuffer_sub:asstring() -> str

        :return str: Computed value.
        :rtype str: string

        Read the sub buffer and convert it to a string.

    .. haka:method:: vbuffer_sub:setstring(value)

        :param value: New value.
        :paramtype value: string

        Replace the sub buffer by the given string. If the string is larger or smaller than the current value,
        the buffer will be extended or shrinked to hold the new value.

    .. haka:method:: vbuffer_sub:setfixedstring(value)

        :param value: New value.
        :paramtype value: string

        Replace, in-place, the sub buffer by the given string. The size of the buffer will not change.


Iterator
--------

.. haka:class:: vbuffer_iterator
    :module:

    Iterator on a buffer. An iterator can be *blocking* when working on a stream. In this case, some functions
    can block waiting for more data to be available.

    .. haka:method:: vbuffer_iterator:mark(readonly = false)

        :param readonly: State of the mark.
        :paramtype readonly: boolean

        Create a mark in the buffer at the iterator position.

    .. haka:method:: vbuffer_iterator:unmark()

        Remove a mark in the buffer. The iterator must point to a previously
        created mark otherwise an error will be raised.

    .. haka:method:: vbuffer_iterator:advance(size) -> relsize

        :param size: Amount of bytes to skip
        :paramtype size: number
        :return relsize: The real amount of bytes skipped. This value can be smaller than size if not enough data are available.
        :rtype relsize: number

        Advance the iterator of the given *size* bytes.

    .. haka:method:: vbuffer_iterator:available() -> size

        :return size: Available bytes.
        :rtype size: number

        Get the amount of bytes available after the iterator position.

    .. haka:method:: vbuffer_iterator:check_available(size) -> enough, size

        :param size: Minimum buffer size to check for.
        :paramtype size: number
        :return enough: ``true`` if the available data are larger or equal to *size*.
        :rtype enough: boolean
        :return size: If *enough* is ``false``, size of the available data.
        :rtype size: number

        Check if the available bytes are larger or equal to a given value.

    .. haka:method:: vbuffer_iterator:insert(data) -> sub

        :param data: Buffer to insert.
        :paramtype data: :haka:class:`vbuffer`
        :return sub: Sub-buffer matching the inserted data in the new buffer.
        :rtype sub: :haka:class:`vbuffer_sub`

        Insert some data at the iterator position.

    .. haka:method:: vbuffer_iterator:restore(data)

        :param data: Buffer to restore.
        :paramtype data: :haka:class:`vbuffer`

        Restore data at the iterator position. This iterator must point to
        the reference returned by the function :haka:func:`<vbuffer_sub>.select()`.

    .. haka:method:: vbuffer_iterator:sub(size, split = false) -> sub

        :param size: Size of the requested sub buffer, ``'available'`` or ``'all'``.
        :paramtype size: number or string
        :param split: If ``true``, a split will be inserted at the end of the sub-buffer (see :haka:func:`<vbuffer_iterator>.split()`).
        :paramtype split: boolean
        :return sub: Created sub buffer.
        :rtype sub: :haka:class:`vbuffer_sub`

        Create a sub buffer from the iterator position.

    .. haka:method:: vbuffer_iterator:split()

        Split the buffer at the iterator position.

    .. haka:method:: vbuffer_iterator:move_to(iter)

        :param iter: Destination iterator.
        :paramtype iter: :haka:class:`vbuffer_iterator`

        Move the iterator to a new position.

    .. haka:method:: vbuffer_iterator:wait() -> eof

        :return eof: ``true`` if the iterator is at the end of the buffer.
        :rtype eof: boolean

        Wait for some data to be available.

    .. haka:method:: vbuffer_iterator:foreach_available() -> loop

        Return a Lua iterator to build a loop getting each available sub-buffer
        one by one.

        **Usage:**

        ::

            for sub in iter:foreach_available() do
                print(#sub)
            end

    .. haka:attribute:: vbuffer_iterator.meter

        :type: number

        Index that can be used to track the offset of the iterator. This index is automatically updated
        when the iterator advance.

    .. haka:attribute:: vbuffer_iterator.iseof
        :readonly:

        :type: boolean

        ``true`` if the iterator is at the end of buffer and no more data can
        be available even later in case of a stream.


Streams
-------

.. haka:class:: vbuffer_stream
    :module:

    A buffer stream is an object that can convert different separated buffer into
    a view where only one buffer is visible. This is for instance used by TCP to
    recreate a stream of data from each received packets.

    .. haka:function:: vbuffer_stream() -> stream

        :return stream: New stream.
        :rtype stream: :haka:class:`vbuffer_stream`

        Create a new buffer stream.

    .. haka:method:: vbuffer_stream:push(data) -> iter

        :param data: Buffer data.
        :paramtype data: :haka:class:`vbuffer`
        :return iter: Iterator pointing to the beginning of the new added data in the stream.
        :rtype iter: :haka:class:`vbuffer_iterator`

        Push some data into the stream.

    .. haka:method:: vbuffer_stream:finish()

        Mark the end of the stream. Any call to :haka:func:`<vbuffer_stream>.push()`
        will result to an error.

    .. haka:method:: vbuffer_stream:pop() -> buffer

        :return buffer: Extracted data from the stream.
        :rtype buffer: :haka:class:`vbuffer`

        Pop available data from the stream.

    .. haka:attribute:: vbuffer_stream.isfinished
        :readonly:

        :type: boolean

        Get the stream finished state.

    .. haka:attribute:: vbuffer_stream.data
        :readonly:

        :type: :haka:class:`vbuffer`

        All data in the stream.


.. haka:class:: vbuffer_sub_stream
    :module:

    A sub stream is an object that will build a stream view from a list a sub-buffer.

    .. haka:function:: vbuffer_sub_stream() -> stream

        :return stream: New stream.
        :rtype stream: :haka:class:`vbuffer_sub_stream`

        Create a new sub-buffer stream.

    .. haka:method:: vbuffer_sub_stream:push(data) -> iter

        :param data: Sub-buffer data.
        :paramtype data: :haka:class:`vbuffer_sub`
        :return iter: Iterator pointing to the beginning of the new added data in the stream.
        :rtype iter: :haka:class:`vbuffer_iterator`

        Push some data into the stream. The sub-buffer will be extracted with a :haka:func:`<vbuffer_sub>.select()`.

    .. haka:method:: vbuffer_sub_stream:finish()

        Mark the end of the stream. Any call to :haka:func:`<vbuffer_sub_stream>.push()`
        will result to an error.

    .. haka:method:: vbuffer_sub_stream:pop() -> sub

        :return sub: Extracted data from the stream.
        :rtype sub: :haka:class:`vbuffer_sub`

        Pop available data from the stream and automatically do a :haka:func:`<vbuffer_iterator>.restore()`.

    .. haka:attribute:: vbuffer_sub_stream.isfinished
        :readonly:

        :type: boolean

        Get the stream finished state.

    .. haka:attribute:: vbuffer_sub_stream.data
        :readonly:

        :type: :haka:class:`vbuffer`

        All data in the stream.


Stream coroutine manager
------------------------

.. haka:class:: vbuffer_stream_comanager
    :module:

    This class allow to execute function inside a coroutine and to be able to block transparently
    if needed when the stream does not have enough data available.
    
    .. haka:method:: vbuffer_stream_comanager:start(id, f)

        :param id: Identifier for the registered function.
        :paramtype id: any
        :param f: Function to be started.
        :paramtype f: function
        
        Register and start a new function on the stream.
    
    .. haka:method:: vbuffer_stream_comanager:has(id) -> found

        :param id: Identifier for the registered function.
        :paramtype id: any
        :return found: ``true`` if the id is fould inside the registered functions.
        :rtype found: boolean
        
        Check if the given *id* match a registered function.
    
    .. haka:method:: vbuffer_stream_comanager:process(id, current)

        :param id: Identifier for the registered function.
        :paramtype id: any
        :param current: Current position in the stream.
        :paramtype current: :haka:class:`vbuffer_iterator`
        
        Resume execution for the registered *id*. This function needs to be called whenever some new
        data are available on this stream. 
    
    .. haka:method:: vbuffer_stream_comanager:process_all(current)
    
        :param current: Current position in the stream.
        :paramtype current: :haka:class:`vbuffer_iterator`
        
        Resume execution for all registered functions.

.. highlightlang:: lua

Packet :lua:mod:`haka.packet`
=============================

.. lua:module:: haka.packet

.. lua:class:: packet

    Object representing a packet.  The data can be accessed using the standard Lua
    operators `#` to get the length and `[]` to access the bytes.

    .. lua:method:: drop()

        Drop the packet.

        .. note:: The object will be unusable after calling this function.

    .. lua:method:: accept()

        Accept the packet.

        .. note:: The object will be unusable after calling this function.

    .. lua:method:: send()

        Send the packet.

    .. lua:method:: resize(size)

        Set the packet length to ``size``.

.. lua:method:: new(size)

    Create new packet of size ``size``.

.. lua:method:: mode()

	Returns the packet mode: passthrough mode or normal mode.

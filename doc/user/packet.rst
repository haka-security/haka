.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Packet
======

.. lua:module:: haka.packet

.. lua:class:: packet

    Object representing a packet.  The data can be accessed using the standard Lua
    operators `#` to get the length and `[]` to access the bytes.

    .. lua:method:: drop()

        Drop the packet.

        .. note:: The packet will be unusable after calling this function.

    .. lua:method:: accept()

        Accept the packet.

        .. note:: The packet will be unusable after calling this function.

    .. lua:method:: send()

        Send the packet.

    .. lua:method:: resize(size)

        Set the packet length to ``size``.

.. lua:method:: new(size)

    Create new packet of size ``size``.

.. lua:method:: mode()

	Returns the packet mode: passthrough mode or normal mode.

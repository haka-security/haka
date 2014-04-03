.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Packet
======

.. lua:module:: haka.packet

.. lua:class:: packet

    The Packet class represents a raw data packet. I.e a blob of binary data coming from the capture module.

    Nothing is know of its content. 

    Packet objects are passed to the first disector in the stack to analyze the content.

    .. lua:method:: []
        
        individual bytes within the packet can be obtained by indexing the packet objet

    .. lua:method:: #
       
        The length of the packet object can be obtained via the normal lua operators


    .. lua:method:: drop(self)

        Drop the packet.

        .. note:: The packet will be unusable after calling this function.

        :param self: The packet to drop
        :paramtype self: :lua:class:`packet`

    .. lua:method:: accept(self)

        Accept the packet.

        .. note:: The packet will be unusable after calling this function.

        :param self: The packet to accept
        :paramtype self: :lua:class:`packet`

    .. lua:method:: send(self)

        Send the packet.

        :param self: The packet to send
        :paramtype self: :lua:class:`packet`

    .. lua:method:: resize(self,size)

        Set the packet length to ``size``.

        :param self: The packet whose size will be changed
        :paramtype self: :lua:class:`packet`
        :param size: The new size of the packet
        :paramtype size: `ìnteger`

.. lua:function:: new(size)

    Create new packet of size ``size``.

    :param size: The new size of the packet
    :paramtype size: `ìnteger`

.. lua:function:: mode()

        :returns: the packet mode: passthrough mode or normal mode.
        :rtype: integer

.. lua:data:: NORMAL

   constant integer representing the normal mode

.. lua:data:: PASSTHROUGH

   constant integer representing the passthrough mode

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Packet
======

.. haka:module:: haka

.. haka:class:: packet
    :module:

    Object that represents a raw data packet. I.e a blob of binary data coming from the capture module.
    Nothing is know of its content.

    .. haka:function:: packet(size = 0) -> pkt

        :param size: Size of the new packet.
        :paramtype size: number
        :return pkt: New packet.
        :rtype pkt: :haka:class:`packet`
    
        Create a new packet of the given size.

    .. haka:attribute:: packet.payload
        :readonly:

        :type: :haka:class:`vbuffer` |nbsp|

        Data inside the packet.

    .. haka:method:: packet:drop()

        Drop the packet.

        .. note:: The packet will be unusable after calling this function.

    .. haka:method:: packet:send()

        Send the packet on the network.

        .. note:: The packet will be unusable after calling this function.

    .. haka:method:: packet:inject()

        Re-inject the packet. The packet will re-enter the Haka rules and dissector
        exactly like a new packet coming from the capture module.

    .. haka:method:: state() -> state

        :return state: State of the packet: ``'forged'``, ``'normal'`` or ``'sent'``.
        :rtype state: string

        Get the state of the packet.

.. haka:function:: packet_mode() -> mode

    :return mode: Current packet mode (``'normal'`` or ``'passthrough'``).
    :rtype mode: string

    Get the current packet mode for Haka. In *passthrough* mode, the packet cannot
    be modified nor dropped.

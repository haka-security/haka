.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Packet
======

.. haka:module:: haka

.. haka:class:: packet
    :module:
    :objtype: dissector

    :Name: ``'packet'``
    :Extend: :haka:class:`haka.helper.PacketDissector`

    Object that represents a data packet. I.e a blob of binary data coming from the capture module.
    Nothing is known about its content.

    .. haka:function:: create(size = 0) -> pkt

        :param size: Size of the new packet.
        :paramtype size: number
        :return pkt: packet dissector.
        :rtype pkt: :haka:class:`packet`

        Create a new packet.

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

.. haka:function:: capture_mode() -> mode

    :return mode: Current packet capture mode (``'normal'`` or ``'passthrough'``).
    :rtype mode: string

    Get the current packet capture mode for Haka. In *passthrough* mode, the packet cannot
    be modified nor dropped.


Events
------

.. haka:function:: packet.events.receive_packet(pkt)
    :module:
    :objtype: event

    :param pkt: packet.
    :paramtype pkt: :haka:class:`packet`

    Event that is triggered whenever a new packet is received.

.. haka:function:: packet.events.send_packet(pkt)
    :module:
    :objtype: event

    :param pkt: packet.
    :paramtype pkt: :haka:class:`packet`

    Event that is triggered just before sending a packet on the network.

Example
-------

::

    local pkt = haka.dissector.packet.create(150)
    print(#pkt.payload)
    pkt:send()

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Udp
===

.. haka:module:: udp

Udp state-less dissector module. ::

    local udp = require('protocol/udp')

.. haka:class:: UdpDissector
    :module:
    :objtype: dissector

    :Name: ``'udp'``
    :Extend: :haka:class:`haka.dissector.PacketDissector` |nbsp|

    Dissector data for a UDP packet.

    .. haka:function:: create(ip[, init]) -> udp

        :param ip: IPv4 packet.
        :paramtype ip: :haka:class:`Ipv4Dissector`
        :param init: Optional initialization data.
        :paramtype init: table
        :return udp: Created packet.
        :rtype udp: :haka:class:`UdpDissector`

        Create a new UDP packet on top of the given IP packet.

    .. haka:attribute:: UdpDissector:srcport
                        UdpDissector:dstport
                        UdpDissector:length
                        UdpDissector:checksum

        :type: number

        UDP fields.

    .. haka:attribute:: UdpDissector:payload

        :type: :haka:class:`vbuffer` |nbsp|

        Payload of the packet.

    .. haka:method:: UdpDissector:verify_checksum() -> correct

        :return correct: ``true`` if the checksum is correct.
        :rtype correct: boolean

        Verify if the checksum is correct.


Events
------

.. haka:function:: udp.events.receive_packet(pkt)
    :module:
    :objtype: event

    :param pkt: UDP packet.
    :paramtype pkt: :haka:class:`UdpDissector`

    Event that is triggered whenever a new packet is received.

.. haka:function:: udp.events.send_packet(pkt)
    :module:
    :objtype: event

    :param pkt: UDP packet.
    :paramtype pkt: :haka:class:`UdpDissector`

    Event that is triggered just before sending a packet on the network.

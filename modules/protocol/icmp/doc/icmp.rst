.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

ICMP
====

.. haka:module:: icmp

.. haka:class:: IcmpDissector
    :module:
    :objtype: dissector

    :Name: ``'icmp'``
    :Extend: :haka:class:`PacketDissector` |nbsp|

    Dissector data for an ICMP packet.

    .. haka:function:: create(ip[, init]) -> icmp

        :param ip: IPv4 packet.
        :paramtype ip: :haka:class:`Ipv4Dissector`
        :param init: Optional initialization data.
        :paramtype init: table
        :return icmp: Created packet.
        :rtype icmp: :haka:class:`IcmpDissector`
    
        Create a new ICMP packet on top of the given IP packet.

    .. haka:attribute:: IcmpDissector:type
                        IcmpDissector:code
                        IcmpDissector:checksum

        :type: number
        
        ICMP fields.

    .. haka:attribute:: IcmpDissector:payload

        :type: :haka:class:`vbuffer` |nbsp|
        
        Payload of the packet.


Events
------

.. haka:function:: tcp.events.receive_packet(pkt)
    :module:
    :objtype: event
    
    :param pkt: ICMP packet.
    :paramtype pkt: :haka:class:`IcmpDissector`
    
    Event that is triggered whenever a new packet is received.

.. haka:function:: tcp.events.send_packet(pkt)
    :module:
    :objtype: event
    
    :param pkt: ICMP packet.
    :paramtype pkt: :haka:class:`IcmpDissector`
    
    Event that is triggered just before sending a packet on the network.

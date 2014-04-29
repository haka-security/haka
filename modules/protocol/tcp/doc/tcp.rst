.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Tcp
===

.. haka:module:: tcp

Tcp state-less dissector module.

**Usage:**

::

    local tcp = require('protocol/tcp')

Dissector
---------

.. haka:class:: TcpDissector
    :module:
    :objtype: dissector

    :Name: ``'tcp'``
    :Extend: :haka:class:`haka.dissector.PacketDissector` |nbsp|

    Dissector data for a TCP packet. This dissector is state-less. It will only parse the
    packet headers.

    .. haka:function:: create(ip) -> tcp

        :param ip: IPv4 packet.
        :paramtype ip: :haka:class:`Ipv4Dissector`
        :return tcp: Created packet.
        :rtype tcp: :haka:class:`TcpDissector`

        Create a new TCP packet on top of the given IP packet.

    .. haka:attribute:: TcpDissector:srcport
                        TcpDissector:dstport
                        TcpDissector:seq
                        TcpDissector:ack_seq
                        TcpDissector:res
                        TcpDissector:hdr_len
                        TcpDissector:window_size
                        TcpDissector:checksum
                        TcpDissector:urgent_pointer

        :type: number

        TCP fields.

    .. haka:attribute:: TcpDissector:flags.fin
                        TcpDissector:flags.syn
                        TcpDissector:flags.rst
                        TcpDissector:flags.psh
                        TcpDissector:flags.ack
                        TcpDissector:flags.urg
                        TcpDissector:flags.ecn
                        TcpDissector:flags.cwr

        :type: boolean

        TCP flags.

    .. haka:attribute:: TcpDissector:flags.all

        :type: number

        All flags raw value.

    .. haka:attribute:: TcpDissector:payload

        :type: :haka:class:`vbuffer` |nbsp|

        Payload of the packet.

    .. haka:attribute:: TcpDissector:ip
        :readonly:

        :type: :haka:class:`Ipv4Dissector` |nbsp|

        IPv4 packet.

    .. haka:method:: TcpDissector:verify_checksum() -> correct

        :return correct: ``true`` if the checksum is correct.
        :rtype correct: boolean

        Verify if the checksum is correct.

    .. haka:method:: TcpDissector:compute_checksum()

        Recompute the checksum and set the resulting value in the packet.

    .. haka:method:: TcpDissector:drop()

        Drop the TCP packet.

    .. haka:method:: TcpDissector:send()

        Send the packet.

    .. haka:method:: TcpDissector:inject()

        Inject the packet.

Events
------

.. haka:function:: tcp.events.receive_packet(pkt)
    :module:
    :objtype: event

    :param pkt: TCP packet.
    :paramtype pkt: :haka:class:`TcpDissector`

    Event that is triggered whenever a new packet is received.

.. haka:function:: tcp.events.send_packet(pkt)
    :module:
    :objtype: event

    :param pkt: TCP packet.
    :paramtype pkt: :haka:class:`TcpDissector`

    Event that is triggered just before sending a packet on the network.


Utilities
---------

.. warning:: This section contains advanced feature of Haka.

.. haka:class:: tcp_stream
    :module:

    TCP stream helper object.

    .. haka:function:: tcp_stream() -> stream

        :return stream: New TCP stream.
        :rtype stream: :haka:class:`tcp_stream`

        Create a new TCP stream.

    .. haka:method:: tcp_stream:init(seq)

        :param seq: Initial sequence number for this stream.
        :paramtype seq: number

        Initialize the initial sequence number of the stream.

    .. haka:method:: tcp_stream:push(tcp)

        :param tcp: TCP packet.
        :paramtype tcp: :haka:class:`TcpDissector`

        Push a tcp packet into the stream.

    .. haka:method:: tcp_stream:pop() -> tcp

        :return tcp: TCP packet.
        :rtype tcp: :haka:class:`TcpDissector`

        Pop a tcp packet out of the stream.

    .. haka:method:: tcp_stream:seq(tcp)

        :param tcp: TCP packet.
        :paramtype tcp: :haka:class:`TcpDissector`

        Update the sequence number of a tcp packet.

    .. haka:method:: tcp_stream:ack(tcp)

        :param tcp: TCP packet.
        :paramtype tcp: :haka:class:`TcpDissector`

        Update the ack number of a packet.

    .. haka:method:: tcp_stream:clear()

        Clear the stream and drop all remaining packet.

    .. haka:attribute:: tcp_stream:stream

        :type: :haka:class:`vbuffer_stream`

        Associated raw stream.

    .. haka:attribute:: tcp_stream:lastseq
        :readonly:

        :type: number

        Last received sequence number.

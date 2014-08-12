.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Ipv4
====

.. haka:module:: ipv4

Ipv4 dissector module.

**Usage:**

::

    local ipv4 = require('protocol/ipv4')

Dissector
---------

.. haka:class:: Ipv4Dissector
    :module:
    :objtype: dissector

    :Name: ``'ipv4'``
    :Extend: :haka:class:`haka.helper.PacketDissector` |nbsp|

    IP version 4 packet dissector.

    .. haka:function:: register_proto(proto, dissector)

        :param proto: IP protocol number.
        :paramtype proto: number
        :param dissector: Dissector to use.
        :paramtype dissector: :haka:class:`Dissector`

        Register the dissector to associate with the given IP protocol number.

    .. haka:function:: create(pkt) -> ip

        :param pkt: Lower level packet.
        :paramtype pkt: dissector
        :return ip: Created packet.
        :rtype ip: :haka:class:`Ipv4Dissector`

        Create a new IPv4 packet on top of a lower level packet (raw for instance).

    .. haka:attribute:: Ipv4Dissector:hdr_len
                        Ipv4Dissector:version
                        Ipv4Dissector:tos
                        Ipv4Dissector:len
                        Ipv4Dissector:id
                        Ipv4Dissector:frag_offset
                        Ipv4Dissector:ttl
                        Ipv4Dissector:proto
                        Ipv4Dissector:checksum

        :type: number

        IPv4 fields.

    .. haka:attribute:: Ipv4Dissector:src
                        Ipv4Dissector:dst

        :type: :haka:class:`addr` |nbsp|

        Source and destination.

    .. haka:attribute:: Ipv4Dissector:flags.rb
                        Ipv4Dissector:flags.df
                        Ipv4Dissector:flags.mf

        :type: boolean

        IPv4 flags.

    .. haka:attribute:: Ipv4Dissector.flags.all

        :type: number

        All flags raw value.

    .. haka:attribute:: Ipv4Dissector.payload

        :type: :haka:class:`vbuffer` |nbsp|

        Payload of the packet.

    .. haka:method:: Ipv4Dissector:verify_checksum() -> correct

        :return correct: ``true`` if the checksum is correct.
        :rtype correct: boolean

        Verify if the checksum is correct.

    .. haka:method:: Ipv4Dissector:compute_checksum()

        Recompute the checksum and set the resulting value in the packet.

    .. haka:method:: Ipv4Dissector:drop()

        Drop the packet.

    .. haka:method:: Ipv4Dissector:send()

        Send the packet.

    .. haka:method:: Ipv4Dissector:inject()

        Inject the packet.

Events
------

.. haka:function:: ipv4.events.receive_packet(pkt)
    :module:
    :objtype: event

    :param pkt: IPv4 packet.
    :paramtype pkt: :haka:class:`Ipv4Dissector`

    Event that is triggered whenever a new packet is received.

.. haka:function:: ipv4.events.send_packet(pkt)
    :module:
    :objtype: event

    :param pkt: IPv4 packet.
    :paramtype pkt: :haka:class:`Ipv4Dissector`

    Event that is triggered just before sending a packet on the network.


Utilities
---------

.. haka:class:: addr
    :module:

    Represent an ipv4 address.

    .. haka:function:: addr(str) -> addr
                       addr(addr) -> addr
                       addr(a, b, c, d) -> addr

        :param str: IP address as a string representation (ie. ``'127.0.0.1'``)
        :paramtype str: string
        :param addr: IP address as a number representation (ie. ``0x0100007f``)
        :paramtype addr: number
        :param a,b,c,d: IP address as a byte representation
        :paramtype addr: number
        :return addr: Created address.
        :rtype addr: :haka:class:`addr`

        Address constructors.

        **Examples:** ::

            ipv4.addr("127.0.0.1")
            ipv4.addr(0x0100007f)
            ipv4.addr(127, 0, 0, 1)

    .. haka:attribute:: addr.packed

        :type: number

        Packed representation of the IP address.

    .. haka:operator:: tostring(addr) -> str

        :return str: String representation of the address.
        :rtype str: string

        Convert an address to its string representation.

.. haka:class:: network
    :module:

    Class used to represent an ipv4 network address.

    .. haka:function:: network(str) -> net
                       network(ipaddr, mask) -> net

        :param str: String representation of the IP network (ie. ``'127.0.0.1/8'``).
        :paramtype str: string
        :param ipaddr: IP network address.
        :paramtype ipaddr: :haka:class:`addr`
        :param mask: IP network mask.
        :paramtype mask: number
        :return net: New IP network.
        :rtype net: :haka:class:`network`

        Network constructors.

        **Examples:** ::

            ipv4.network("127.0.0.1/8")
            ipv4.network(ipv4.addr(127, 0, 0, 1), 8)

    .. haka:attribute:: network.net
        :readonly:

        :type: :haka:class:`addr` |nbsp|

        Network address.

    .. haka:attribute:: network.mask
        :readonly:

        :type: number

        Network mask.

    .. haka:method:: network:contains(addr) -> bool

        :param addr: An IP address
        :ptype addr: :haka:class:`addr`
        :return bool: true if IP address belong to the network, false otherwise.
        :rtype bool: boolean

        Check if the IP address belong to the network.

    .. haka:operator:: tostring(network) -> str

        :return str: String representation of the network.
        :rtype str: string

        Convert a network to its string representation.

.. haka:class:: inet_checksum
    :module:

    Helper to compute inet checksum on buffers pieces by pieces.

    .. haka:function:: checksum_partial() -> new

        :return new: New inet checksum helper.
        :rtype new: :haka:class:`inet_checksum`

        Create a new inet checksum helper.

    .. haka:method:: process(buffer)
                     process(sub)

        :param buffer: Buffer to process.
        :paramtype buffer: :haka:class:`vbuffer` |nbsp|
        :param sub: Sub-buffer to process.
        :paramtype sub: :haka:class:`vbuffer_sub` |nbsp|

        Process the buffer to compute its checksum value. This function can be called
        multiple times to compute it on data represented by multiple buffers.

    .. haka:method:: reduce() -> checksum

        :return checksum: Final checksum value.
        :rtype checksum: number

        Compute the final inet checksum value. This function must be called at the end
        after one or multiple calls to :haka:func:`process`.


.. haka:class:: cnx_table
    :module:

    Object used to create a table of connections. The connection table uses source and
    destination IP along with some source and destination ports. Those ports can be extracted
    from TCP or UDP for instance.

    .. haka:function:: cnx_table() -> table

        :return table: New connection table
        :rtype table: :haka:class:`cnx_table`

        Create a new connection table.

    .. haka:method:: cnx_table:create(srcip, dstip, srcport, dstport) -> cnx

        :param srcip: Source IP.
        :paramtype srcip: :haka:class:`addr`
        :param dstip: Destination IP.
        :paramtype dstip: :haka:class:`addr`
        :param srcport: Source port.
        :paramtype srcport: number
        :param dstport: Destination port.
        :paramtype dstport: number
        :return cnx: New connection
        :rtype cnx: :haka:class:`cnx`

        Create a new entry in the connection table.

    .. haka:method:: cnx_table:get(srcip, dstip, srcport, dstport) -> cnx

        :param srcip: Source IP.
        :paramtype srcip: :haka:class:`addr`
        :param dstip: Destination IP.
        :paramtype dstip: :haka:class:`addr`
        :param srcport: Source port.
        :paramtype srcport: number
        :param dstport: Destination port.
        :paramtype dstport: number
        :return cnx: Corresponding connection
        :rtype cnx: :haka:class:`cnx`

        Get an entry in the connection table.

.. haka:class:: cnx
    :module:

    Object that represent a connection.

    .. haka:attribute:: cnx:data

        Data that can be used to associate any Lua object with the connection.

    .. haka:method:: cnx:close()

        Close the connection. It will be removed from the associated table.

    .. haka:method:: cnx:drop()

        Mark the connection as dropped. The connection remains in the table until
        :haka:func:`<cnx>.close()` is called.


Example
-------

.. literalinclude:: ../../../../sample/ruleset/ipv4/security.lua
    :language: lua
    :tab-width: 4

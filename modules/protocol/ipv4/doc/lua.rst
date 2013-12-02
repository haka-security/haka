
.. highlightlang:: lua

IPv4
====

.. lua:module:: ipv4

Types
-----

.. lua:class:: addr

    Class used to represent an ipv4 address.

    .. lua:method:: addr(str)
                    addr(addr)
                    addr(a, b, c, d)

        Address constructors from string value, number or byte numbers.

        Examples: ::

            ipv4.addr("127.0.0.1")
            ipv4.addr(127, 0, 0, 1)

.. lua:class:: network

    Class used to represent an ipv4 network address.

    .. lua:method:: network(str)
                    network(ipaddr, mask)

        Network constructors from string value, number or byte numbers.

        Examples: ::

            ipv4.network("127.0.0.1/8")
            ipv4.network(ipv4.addr(127, 0, 0, 1), 8)

    .. lua:data:: net

        Network address.

        .. note:: This field is read-only.

    .. lua:data:: mask

        Network mask.

        .. note:: This field is read-only.


Functions
---------

.. lua:function:: register_proto(proto, name)

    Register the dissector to associate with the given protocol `proto` number.


Dissector
---------

This module register the `ipv4` dissector.

.. lua:function:: create(raw)

    Create a new IPv4 packet on top of the given raw packet.

.. lua:class:: ipv4

    Dissector data for an ipv4 packet.

    .. seealso:: :lua:class:`haka.dissector_data`.

    .. lua:data:: hdr_len
                  version
                  tos
                  len
                  id
                  frag_offset
                  ttl
                  proto
                  checksum

        IPv4 fields as numbers.

    .. lua:data:: src
                  dst

        Source and destination as :lua:class:`ipv4.addr`.

    .. lua:data:: flags

        IPv4 flags table.

        .. lua:data:: rb
                      df
                      mf

            Individual flags as boolean.

        .. lua:data:: all

            Flags value as number.

    .. lua:data:: payload

        Payload of the packet. Class that contains the ipv4 data payload. The data can be
        accessed using the standard Lua operators `#` to get the length and `[]` to access
        the bytes.

    .. lua:method:: verify_checksum()

        Verify if the checksum is correct.

    .. lua:method:: compute_checksum()

        Recompute the checksum and set the resulting value in the packet.

    .. lua:method:: drop()

        Drop the IPv4 packet.

Example
-------

.. literalinclude:: ../../../../sample/ruleset/ipv4/security.lua
    :language: lua
    :tab-width: 4

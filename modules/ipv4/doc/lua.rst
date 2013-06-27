
.. highlightlang:: lua

IPv4
====

.. lua:module:: ipv4

Types
-----

.. lua:class:: addr

.. lua:function:: ipv4.addr(str)
                  ipv4.addr(addr)
                  ipv4.addr(a, b, c, d)

    Address constructors from string value, number or byte numbers.

    Examples: ::

        ipv4.addr("127.0.0.1")
        ipv4.addr(127, 0, 0, 1)

.. lua:class:: network

    .. lua:data:: net

        Network address.

        .. note:: This field is read-only.

    .. lua:data:: mask

        Network mask.

        .. note:: This field is read-only.

.. lua:function:: ipv4.network(str)
                  ipv4.network(ipaddr, mask)

    Network constructors from string value, number or byte numbers.

    Examples: ::

        ipv4.network("127.0.0.1/8")
        ipv4.network(ipv4.addr(127, 0, 0, 1), 8)


Functions
---------

.. lua:function:: register_proto(proto, name)

   Register the dissector to associate with the given protocol `proto`.


Dissector
---------

This module register the `ipv4` dissector.


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

        IPv4 flags.

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

Example
-------

.. literalinclude:: ../../../sample/standard/proto/ipv4/security.lua
    :language: lua

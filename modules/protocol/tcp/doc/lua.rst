
.. highlightlang:: lua

TCP
===

.. lua:module:: tcp


Types
-----

.. lua:class:: tcp_connection

    .. lua:data:: srcip
                  dstip

        Source and destination IP addresses.

    .. lua:data:: srcport
                  dstport

        Source and destination ports.

    .. lua:data:: data

        A user field that can be used to store some data associated
        with the connection.


.. lua:class:: tcp_stream

    .. lua:method:: init(seq)

        Initialize the initial sequence number of the stream.

    .. lua:method:: push(tcp)

        Push a tcp packet into the stream.

    .. lua:method:: pop()

        Pop a tcp packet out of the stream.

    .. lua:method:: seq(tcp)

        Update the sequence number of a tcp packet.

    .. lua:method:: ack(tcp)

        Update the ack number of a packet.



Dissectors
----------

TCP
^^^

.. lua:function:: create(ip)

    Create a new TCP packet on top of the given IP packet.

.. lua:class:: tcp

    Dissector data for a TCP packet. This dissector is state-less. It will only parse the
    packet headers.

    .. seealso:: :lua:class:`haka.dissector_data`.

    .. lua:data:: srcport
                  dstport
                  seq
                  ack_seq
                  res
                  hdr_len
                  window_size
                  checksum
                  urgent_pointer

        TCP fields as numbers.

    .. lua:data:: ip

        IPv4 packet.

    .. lua:data:: flags

        TCP flags.

        .. lua:data:: fin
                      syn
                      rst
                      psh
                      ack
                      urg
                      ecn
                      cwr

            Individual flags as boolean.

        .. lua:data:: all

            Flags value as number.

    .. lua:data:: payload

        Payload of the packet. Class that contains the TCP data payload. The data can be
        accessed using the standard Lua operators `#` to get the length and `[]` to access
        the bytes.

    .. lua:method:: verify_checksum()

        Verify if the checksum is correct.

    .. lua:method:: compute_checksum()

        Recompute the checksum and set the resulting value in the packet.

    .. lua:method:: newconnection()

        Creates a new TCP connection from this packet.

    .. lua:method:: getconnection()

        Gets the connection if any associated with this packet.

        :returns:
            * The TCP connection as :lua:class:`tcp.tcp_connection`.
            * A boolean containing the direction of this packet in the connection.
            * A boolean indicating if the packet is part of a dropped connection.

    .. lua:method:: drop()

        Drop the TCP packet.


TCP connection
^^^^^^^^^^^^^^

.. lua:module:: tcp-connection

.. lua:class:: connection

    State-full dissector for TCP which define one additional hook `tcp-connection-new` that
    is called whenever a packet will create a new TCP connection.

    .. seealso:: :lua:class:`haka.dissector_data`.

    .. lua:data:: connection

        Contains the current TCP connection.

    .. lua:data:: stream

        Contains the stream associated with the connection.

    .. lua:data:: direction

        Contains the direction of the current packet.

    .. lua:method:: drop()

        Drop the TCP connection. All future packets that belong to this connection will be
        silently dropped.

    .. lua:method:: reset()

        Reset the TCP connection. A RST packet will be sent to both end and all future packet
        that belong to this connection will be silently dropped.

Example
^^^^^^^

.. literalinclude:: ../../../../sample/ruleset/tcp/rules.lua
    :language: lua
    :tab-width: 4

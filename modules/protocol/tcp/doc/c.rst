.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: c

TCP
===

Types
-----

.. c:type:: struct tcp

    Opaque TCP dissector data.

.. c:type:: struct tcp_connection

    TCP connection structure.

.. c:type:: struct tcp_stream

    TCP stream structure.


Functions
---------

TCP connection
^^^^^^^^^^^^^^

.. c:function:: struct tcp_connection *tcp_connection_new(const struct tcp *tcp)

    Create a new TCP connection for the given TCP packet. `tcp` is the packet going from
    the client to the server.

.. c:function:: struct tcp_connection *tcp_connection_get(const struct tcp *tcp, bool *direction_in, bool *dropped)

    Get the TCP connection if any associated with the given TCP packet.

    :param direction_in: Filled with the direction of the given tcp packet. It
        is set to true if the packet follow the input direction, false otherwise.
    :param dropped: Filled with true if the connection have previously been dropped.

.. c:function:: struct stream *tcp_connection_get_stream(struct tcp_connection *conn, bool direction_in)

    Get the stream associated with a TCP connection.

    :param direction_in: Stream direction, input if true, output otherwise.

.. c:function:: void tcp_connection_close(struct tcp_connection *tcp_conn)

    Close the TCP connection.

.. c:function:: void tcp_connection_drop(struct tcp_connection *tcp_conn)

    Drop the TCP connection.

TCP stream
^^^^^^^^^^

.. c:function:: struct stream *tcp_stream_create()

    Create a new tcp stream.

.. c:function:: bool tcp_stream_push(struct stream *stream, struct tcp *tcp)

    Push data into a tcp stream.

    :returns: `true` if successful, `false` otherwise (see :c:func:`clear_error` to get more
        details about the error).

.. c:function:: struct tcp *tcp_stream_pop(struct stream *stream)

    Pop data from a tcp stream.

    :returns: A tcp packet if available. This function will pop all packets that
        have data before the current position in the stream.

.. c:function:: void tcp_stream_init(struct stream *stream, uint32 seq)

    Initialize the stream sequence number. This function must be called before starting pushing packet
    into the stream.

.. c:function:: void tcp_stream_ack(struct stream *stream, struct tcp *tcp)

    Offset the ack number of the packet.

.. c:function:: void tcp_stream_seq(struct stream *stream, struct tcp *tcp)

    Offset the seq number of the packet.

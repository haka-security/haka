
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

TCP
^^^

.. c:function:: struct tcp *tcp_dissect(struct ipv4 *packet)

    Dissect TCP packet.

.. c:function:: struct ipv4 *tcp_forge(struct tcp *packet)

    Forge IPv4 packet from TCP.

.. c:function:: void tcp_release(struct tcp *packet)

    Release TCP structure memory allocation.

.. c:function:: void tcp_pre_modify(struct tcp *packet)

    Make TCP data modifiable.

.. c:function:: void tcp_compute_checksum(struct tcp *packet)

    Compute TCP checksum according to RFC #1071.

.. c:function:: bool tcp_verify_checksum(const struct tcp *packet)

    Verify TCP checksum.

.. c:function:: const uint8 *tcp_get_payload(const struct tcp *packet)

    Get TCP payload data.

.. c:function:: uint8 *tcp_get_payload_modifiable(struct tcp *packet)

    Get TCP modifiable payload data.

.. c:function:: size_t tcp_get_payload_length(const struct tcp *packet)

    Get TCP payload length.

.. c:function:: uint8 *tcp_resize_payload(struct tcp *packet, size_t size)

    Resize the TCP packet.

.. c:function:: void tcp_action_drop(struct tcp *packet)

    Drop the TCP packet.

.. c:function:: bool tcp_valid(struct tcp *packet)

    Get if the packet is valid and can continue to be processed.

.. c:function:: uint16 tcp_get_srcport(const struct tcp *tcp)
                uint8 tcp_get_dstport(const struct tcp *tcp)
                uint32 tcp_get_seq(const struct tcp *tcp)
                uint8 tcp_get_ack_seq(const struct tcp *tcp)
                uint8 tcp_get_res(const struct tcp *tcp)
                uint8 tcp_get_window_size(const struct tcp *tcp)
                uint8 tcp_get_urgent_pointer(const struct tcp *tcp)
                uint16 tcp_get_checksum(const struct tcp *tcp)
                uint8 tcp_get_hdr_len(const struct tcp *tcp)
                uint8 tcp_get_flags(const struct tcp *tcp)
                uint8 tcp_get_flags_fin(const struct tcp *tcp)
                uint8 tcp_get_flags_syn(const struct tcp *tcp)
                uint8 tcp_get_flags_rst(const struct tcp *tcp)
                uint8 tcp_get_flags_psh(const struct tcp *tcp)
                uint8 tcp_get_flags_ack(const struct tcp *tcp)
                uint8 tcp_get_flags_urg(const struct tcp *tcp)
                uint8 tcp_get_flags_ecn(const struct tcp *tcp)
                uint8 tcp_get_flags_cwr(const struct tcp *tcp)

    TCP accessors.

.. c:function:: void tcp_set_srcport(struct tcp *tcp, uint16 v)
                void tcp_set_dstport(struct tcp *tcp, uint16 v)
                void tcp_set_seq(struct tcp *tcp, uint32 v)
                void tcp_set_ack_seq(struct tcp *tcp, uint32 v)
                void tcp_set_res(struct tcp *tcp, uint8 v)
                void tcp_set_window_size(struct tcp *tcp, uint16 v)
                void tcp_set_urgent_pointer(struct ipv4 *ip, uint16 v)
                void tcp_set_checksum(struct tcp *tcp, uint16 v)
                void tcp_set_hdr_len(struct tcp *tcp, uint8 v)
                void tcp_set_flags(struct tcp *tcp, uint8 v)
                void tcp_set_flags_fin(struct tcp *tcp, bool v)
                void tcp_set_flags_syn(struct tcp *tcp, bool v)
                void tcp_set_flags_rst(struct tcp *tcp, bool v)
                void tcp_set_flags_psh(struct tcp *tcp, bool v)
                void tcp_set_flags_ack(struct tcp *tcp, bool v)
                void tcp_set_flags_urg(struct tcp *tcp, bool v)
                void tcp_set_flags_ecn(struct tcp *tcp, bool v)
                void tcp_set_flags_cwr(struct tcp *tcp, bool v)

    TCP setters.

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

.. c:function:: uint16 tcp_connection_get_srcport(const struct tcp_connection *tcp_conn)
                uint16 tcp_connection_get_dstport(const struct tcp_connection *tcp_conn)
                ipv4addr tcp_connection_get_srcip(const struct tcp_connection *tcp_conn)
                ipv4addr tcp_connection_get_dstip(const struct tcp_connection *tcp_conn)

    TCP connection accessors.

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

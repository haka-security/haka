
.. highlightlang:: c

Packet
======

.. c:type:: struct packet

    Opaque packet structure.

.. c:type:: filter_result

    .. c:var:: FILTER_ACCEPT
               FILTER_DROP

.. c:function:: size_t packet_length(struct packet *pkt)

    Get the length of a packet.

.. c:function:: const uint8 *packet_data(struct packet *pkt)

    Get the data of a packet.

.. c:function:: const char *packet_dissector(struct packet *pkt)

    Get packet dissector to use.

.. c:function:: uint8 *packet_data_modifiable(struct packet *pkt)

    Make a packet modifiable.

.. c:function:: int packet_resize(struct packet *pkt, size_t size)

    Resize a packet.

.. c:function:: void packet_drop(struct packet *pkt)

    Drop a packet. The packet will be released and should not be used anymore
    after this call.

.. c:function:: void packet_accept(struct packet *pkt);

    Accept a packet. The packet will be released and should not be used anymore
    after this call.

.. c:function:: int packet_receive(struct packet_module_state *state, struct packet **pkt)

    Receive a packet from the capture module. This function will wait if no packet is
    available.


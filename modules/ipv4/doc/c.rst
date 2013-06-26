
.. highlightlang:: c

IPv4
====

Types
-----

.. c:type:: struct ipv4

    Opaque IPv4 dissector data.


Functions
---------

.. c:function:: struct ipv4 *ipv4_dissect(struct packet *packet)

    Dissect an IPv4 packet.

.. c:function:: struct packet *ipv4_forge(struct ipv4 *ip)

    Forge packets out of an IPv4 packet.

.. c:function:: void ipv4_release(struct ipv4 *ip)

    Release IPv4 structure memory.

.. c:function:: void ipv4_pre_modify(struct ipv4 *ip)

    Function that need to be called before modifying fields not part if the header.
    It is automatically called if you use the standard accessors.

.. c:function:: void ipv4_pre_modify_header(struct ipv4 *ip)

    Function that need to be called before modifying fields of the header.
    It is automatically called if you use the standard header accessors.

.. c:function:: bool ipv4_verify_checksum(const struct ipv4 *ip)

    Verify IPv4 checksum. Returns `true` if checksum is valid and `false` otherwise.

.. c:function:: void ipv4_compute_checksum(struct ipv4 *ip)

    Compute IPv4 checksum according to RFC #1071.

.. c:function:: const uint8 *ipv4_get_payload(struct ipv4 *ip)

    Get IPv4 payload data. This payload should be modified.

.. c:function:: uint8 *ipv4_get_payload_modifiable(struct ipv4 *ip)

    Get IPv4 modifiable payload data.

.. c:function:: size_t ipv4_get_payload_length(struct ipv4 *ip)

    Get IPv4 payload length.

.. c:function:: uint8 *ipv4_resize_payload(struct ipv4 *ip, size_t size)

    Resize the payload and the associated packet.

.. c:function:: const char *ipv4_get_proto_dissector(struct ipv4 *ip)

    Get the protocol dissector name to use for this packet payload.

.. c:function:: void ipv4_register_proto_dissector(uint8 proto, const char *dissector)

    Register the dissector for a given IP protocol number.

.. c:function:: void ipv4_action_drop(struct ipv4 *ip)

    Drop the IP packet

.. c:function:: bool ipv4_valid(struct ipv4 *ip)

    Get if the packet is valid and can continue to be processed.

.. c:function:: int16 inet_checksum(uint16 *ptr, uint16 size)

    Compute standard checksum on the provided data (RFC #107).

    :param ptr: Pointer to the data.
    :param size: Size of input data.

.. c:function:: uint8 ipv4_get_version(const struct ipv4 *ip)
                uint8 ipv4_get_tos(const struct ipv4 *ip)
                uint16 ipv4_get_len(const struct ipv4 *ip)
                uint16 ipv4_get_id(const struct ipv4 *ip)
                uint8 ipv4_get_ttl(const struct ipv4 *ip)
                uint8 ipv4_get_proto(const struct ipv4 *ip)
                uint16 ipv4_get_checksum(const struct ipv4 *ip)
                ipv4addr ipv4_get_src(const struct ipv4 *ip)
                ipv4addr ipv4_get_dst(const struct ipv4 *ip)
                uint8 ipv4_get_hdr_len(const struct ipv4 *ip)
                uint16 ipv4_get_frag_offset(const struct ipv4 *ip)
                uint16 ipv4_get_flags(const struct ipv4 *ip)
                bool ipv4_get_flags_df(const struct ipv4 *ip)
                bool ipv4_get_flags_mf(const struct ipv4 *ip)
                bool ipv4_get_flags_rb(const struct ipv4 *ip)

    IPv4 field accessors.

.. c:function:: void ipv4_set_version(struct ipv4 *ip, uint8 v)
                void ipv4_set_tos(struct ipv4 *ip, uint8 v)
                void ipv4_set_len(struct ipv4 *ip, uint16 v)
                void ipv4_set_id(struct ipv4 *ip, uint16 v)
                void ipv4_get_ttl(struct ipv4 *ip, uint8 v)
                void ipv4_set_proto(struct ipv4 *ip, uint8 v)
                void ipv4_set_checksum(struct ipv4 *ip, uint16 v)
                void ipv4_set_src(struct ipv4 *ip, ipv4addr v)
                void ipv4_set_dst(struct ipv4 *ip, ipv4addr v)
                void ipv4_set_hdr_len(struct ipv4 *ip, uint8 v)
                void ipv4_set_frag_offset(struct ipv4 *ip, uint16 v)
                void ipv4_set_flags(struct ipv4 *ip, uint16 v)
                void ipv4_set_flags_df(struct ipv4 *ip, bool v)
                void ipv4_set_flags_mf(struct ipv4 *ip, bool v)
                void ipv4_set_flags_rb(struct ipv4 *ip, bool v)

    IPv4 field modifiers.

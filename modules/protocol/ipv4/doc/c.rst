
.. highlightlang:: c

IPv4
====

Types
-----

.. c:type:: struct ipv4

    Opaque IPv4 dissector data.

.. c:type:: ipv4addr

    IPv4 address structure.

.. c:type:: ipv4network

    IPv4 network structure.

    .. c:member:: ipv4addr net
    .. c:member:: uint8 mask

Functions
---------

IPv4 address
^^^^^^^^^^^^

.. c:function:: void ipv4_addr_to_string(ipv4addr addr, char *string, size_t size)

    Convert IP from ipv4addr to string.

.. c:function:: ipv4addr ipv4_addr_from_string(const char *string)

    Convert IP from string to ipv4addr structure.

.. c:function:: ipv4addr ipv4_addr_from_bytes(uint8 a, uint8 b, uint8 c, uint8 d)

    Convert IP from bytes to ipv4addr.

IPv4 network
^^^^^^^^^^^^

.. c:var:: const ipv4network ipv4_network_zero

.. c:function:: void ipv4_network_to_string(ipv4network net, char *string, size_t size)

    Convert network address from ipv4network to string.

.. c:function:: ipv4network ipv4_network_from_string(const char *string)

    Convert network address from string to ipv4network structure.

.. c:function:: uint8 ipv4_network_contains(ipv4network network, ipv4addr addr)

    Checks if IPv4 address is in network range.

    :returns: `true` if ip address is in network range and `false` otherwise.

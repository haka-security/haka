
.. program:: packet-nfqueue

Netfilter NFQUEUE capture `packet-nfqueue`
==========================================

The module uses the library netfilter queue to intercept all packets.

At initialization, the module will install some iptables rules for the _raw_ table.
The table will be cleared when the application is closed.

To operate correctly, the process need to be launched with the proper access
rights.

.. option:: -p <input file> <output file> <drop file>

    Optional pcap file used to store, respectively, the received packets, the outputed
    packet and the packets dropped by the filter.

.. option:: <interface1> <interface2> ...

    List of interface to capture.

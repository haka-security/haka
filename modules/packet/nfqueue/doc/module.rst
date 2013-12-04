
Netfilter queue `packet/nfqueue`
================================

Description
^^^^^^^^^^^

The module uses the library netfilter queue to intercept all packets.

At initialization, the module will install some iptables rules for the `raw` table.
The table will be cleared when the application is closed.

To operate correctly, the process need to be launched with the proper access
rights.

Parameters
^^^^^^^^^^

.. describe:: interfaces

    List of comma-separated interfaces.

    Example of possible values :

    .. code-block:: ini

        # Capture loopback traffic
        interfaces = "lo"
        # Capture on interface eth1 and eth2
        # interfaces = "eth1, eth2"
        # Capture on all interfaces
        # interfaces = "any"

.. describe:: dump

    Save output in pcap files (yes/no option).

.. describe:: dump_input

    Save received packets in the specified pcap file capture.

.. describe:: dump_output
    
    Save unfiltered packets in the specified pcap file capture.

.. describe:: dump_drop
    
    Save filtered packets in the specified pcap file capture.

    An example to set packet dumping for nfqueue (only received and filtered packets will be saved in pcap files) :

    .. code-block:: ini

        dump = true
        dump_input = "/tmp/input.pcap"
        dump_drop = "/tmp/drop.pcap"

.. highlightlang:: ini

Pcap  `packet/pcap`
===================

Description
^^^^^^^^^^^

The module uses the library pcap to read packets from a file or from a real network
device.

.. note:
    To be able to capture packets on a real interface, the process need to be launched with
    the proper access rights.

Parameters
^^^^^^^^^^

.. describe:: interfaces

    List of comma-separated interfaces.

    Example of possible values: ::

        # Capture loopback traffic
        interfaces = "lo"
        # Capture loopback traffic and eth0
        # interfaces = "lo, eth0"
        # Capture on all interfaces
        # interfaces = "any"

.. describe:: file

    Read packets from a pcap file. ``interfaces`` must be not be used.

.. describe:: output

    Save unfiltered packets to the specified pcap output file.

    Example of capturing packets from a pcap file and saving unfiltered ones in a pcap output file: ::

        file = "/tmp/input.pcap"
        output = "/tmp/output.pcap"


.. program:: packet-pcap

Pcap packet capture `packet-pcap`
=================================

The module uses the library pcap to read packets from a file or from an real network
device.

To be able to capture packets on a real interface, the process need to be launched with
the proper access rights.

.. option:: -f <pcap file>
            -i <interface name>
            -i any

    Capture file or interface.

.. option:: -o <pcap file>

    Optional output file to save filtered packets.

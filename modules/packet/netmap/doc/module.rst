.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Netmap  `packet/netmap`
===================

Description
^^^^^^^^^^^

The module uses the pcap library to read packets from a pcap file or from a network interface.

.. note:
    To be able to capture packets on a real interface, the process need to be launched with
    the proper permissions.

Parameters
^^^^^^^^^^

.. describe:: interfaces

    Comma-separated list of interfaces or the `any` keyword.

    Example of possible values:

    .. code-block:: ini

        # Capture loopback traffic
        interfaces = "lo"
        # Capture loopback traffic and eth0
        # interfaces = "lo, eth0"
        # Capture on all interfaces
        # interfaces = "any"

.. describe:: file

    Read packets from a pcap file.

.. note::

    Only one of **interfaces** or **file** can be defined.

.. describe:: output=`file`

    Save accepted packets to the specified pcap output file.

    Example of capturing packets from a pcap file and saving accepted ones in a pcap output file:

    .. code-block:: ini

        file = "/tmp/input.pcap"
        output = "/tmp/output.pcap"

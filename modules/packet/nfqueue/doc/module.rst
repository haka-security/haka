.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Netfilter queue `packet/nfqueue`
================================

Description
^^^^^^^^^^^

This module uses the library netfilter queue to capture packets from a given network interface.

This module will install iptable rules in the `raw` table during its initialization
.
The table will be cleared when the application terminates.

When using this module, ``haka`` needs to be run with the appropriate permissions.

Parameters
^^^^^^^^^^

.. describe:: interfaces

    Comma-separated list of interfaces or the `any` keyword.

    Example :

    .. code-block:: ini

        # Capture loopback traffic
        interfaces = "lo"
        # Capture on interface eth1 and eth2
        # interfaces = "eth1, eth2"
        # Capture on all interfaces
        # interfaces = "any"

.. describe:: dump=[yes|no]

    Enable dumping feature.

.. describe:: dump_input=`file`

    Save all received packets to a  pcap file.

.. describe:: dump_output=`file`

    Save packets that were accepted to to a pcap file.

    Example :

    .. code-block:: ini

        dump = true
        dump_input = "/tmp/input.pcap"

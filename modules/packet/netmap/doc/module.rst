.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Netmap  `packet/netmap`
===================

Description
^^^^^^^^^^^

The module uses the netmap kernel module to capture packets from a NIC or host stack of a network interface.

Before starting you have to set up netmap. (see haka+netmap_users_guide)

.. note:
    To be able to capture packets on a real interface, the process need to be launched with
    the proper permissions.

Parameters
^^^^^^^^^^

.. describe:: links

    semicolon-separated list of link between netmap ring pairs.

    Example of possible values:

    .. code-block:: ini

        # Interfaces to plug eth0 NIC to eth0 host stack
        links = "netmap:eth0=netmap:eth0^"

        # Interfaces to plug eth0 NIC to eth1 NIC
        links = "netmap:eth0=netmap:eth1"

	# Interfaces to plug eth0 NIC RX to eth0 NIC TX
        links = "netmap:eth0>netmap:eth0"
"

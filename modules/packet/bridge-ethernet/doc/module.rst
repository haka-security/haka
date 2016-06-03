.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Ethernet  `packet/bridge-ethernet`
==================================

Description
^^^^^^^^^^^

The module captures from either one ethernet port or two (at most).

Parameters
^^^^^^^^^^

.. describe:: devices

    Comma-separated list of devices.

    Example of possible values:

    .. code-block:: ini

        # Capture traffic on one ethernet port
        interfaces = "eth0"
        # Capture traffic and forward it on other port
        #interface = "eth0,eth1"

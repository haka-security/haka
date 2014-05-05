.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Netfilter queue `packet/nfqueue`
================================

Description
-----------

This module uses the `netfilter queue` library to capture packets from a given network interface.

This module will install iptable rules in the `raw` table during its initialization
.
The table will be cleared when the application terminates.

When using this module, ``haka`` needs to be run with the appropriate permissions.


Parameters
----------

.. describe:: interfaces

    Comma-separated list of interfaces or the ``any`` keyword.

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

.. describe:: enable_iptables=[yes|no]

    :Default value: yes

    Enable the iptables rules. If set to ``no``, the user should create
    the rules to select the traffic that should go through ``haka``.

    .. seealso:: :ref:`custom_iptables`.


.. _custom_iptables:

Customize iptables rules
------------------------

By default, Haka will create iptables rules to process all traffic. However it
is possible for the user to customize the rules by using the option
``enable_iptables=no``. In this mode, Haka will only create rules in the
targets ``haka-pre`` and ``haka-out``. It will not change any other rules in
the table `raw` and will rely on the user rules to get some packets.

Haka targets
^^^^^^^^^^^^

You first need to create two custom targets named ``haka-pre`` and ``haka-out``.
These targets will be overridden by Haka when it will be started. Any packet
sent to this target will be processed by Haka.

.. code-block:: console

    # iptables -t raw -N haka-pre
    # iptables -t raw -N haka-out

The target ``haka-pre`` should be used to create rules in PREROUTING,
``haka-out`` should be used in OUTPUT.

Custom rules
^^^^^^^^^^^^

It is then possible to create new rules to send some traffic to Haka. For
instance, the following rule will send all packets:

.. code-block:: console

    # iptables -t raw -A PREROUTING -j haka-pre
    # iptables -t raw -A OUTPUT -j haka-out

.. note::

    This is what Haka do by default or when ``enable_iptables`` is set to
    ``yes``.

Now to only send udp packets to Haka, you can create the following rules:

.. code-block:: console

    # iptables -t raw -A PREROUTING -p udp -j haka-pre
    # iptables -t raw -A OUTPUT -p udp -j haka-out

You can imagine more complex rules using iptables features to select
precisely which packets should by processed and which should not. It
enables seamlessly integration of Haka with iptables.

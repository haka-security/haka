.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Udp Connection
==============

.. haka:module:: udp_connection

Udp state-full dissector module.

**Usage:**

::

    local udp_connection = require('protocol/udp_connection')

Dissector
---------

.. haka:class:: UdpConnectionDissector
    :module:
    :objtype: dissector

    :Name: ``'udp_connection'``
    :Extend: :haka:class:`haka.dissector.FlowDissector` |nbsp|

    State-full dissector for UDP. It will associate each UDP packet with its matching connection.

    .. haka:attribute:: UdpConnectionDissector:srcip
                        UdpConnectionDissector:dstip

        :type: :haka:class:`ipv4.addr` |nbsp|

        Connection IP informations.

    .. haka:attribute:: UdpConnectionDissector:srcport
                        UdpConnectionDissector:dstport

        :type: number

        Connection port informations.

    .. haka:method:: UdpConnectionDissector:drop()

        Drop the UDP connection. All future packets that belong to this connection will be
        silently dropped for a few seconds.


Events
------

.. haka:function:: udp_connection.events.new_connection(flow, udp)
    :module:
    :objtype: event

    :param flow: UDP flow.
    :paramtype flow: :haka:class:`UdpConnectionDissector`
    :param udp: UDP packet.
    :paramtype udp: :haka:class:`UdpDissector`

    Event triggered whenever a new UDP connection is about to be created.

.. haka:function:: udp_connection.events.end_connection(flow)
    :module:
    :objtype: event

    :param flow: UDP flow.
    :paramtype flow: :haka:class:`UdpConnectionDissector`

    Event triggered whenever a new UDP connection is being closed.

.. haka:function:: udp_connection.events.receive_data(flow, payload, direction)
    :module:
    :objtype: event

    :param flow: UDP flow.
    :paramtype flow: :haka:class:`UdpConnectionDissector`
    :param payload: Payload data.
    :paramtype payload: :haka:class:`vbuffer`
    :param direction: Data direction (``'up'`` or ``'down'``).
    :paramtype direction: string

    Event triggered when some data are available on a UDP connection.

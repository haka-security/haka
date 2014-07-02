.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Tcp Connection
==============

.. haka:module:: tcp_connection

Tcp state-full dissector module.

**Usage:**

::

    local tcp_connection = require('protocol/tcp_connection')

Dissector
---------

.. haka:class:: TcpConnectionDissector
    :module:
    :objtype: dissector

    :Name: ``'tcp_connection'``
    :Extend: :haka:class:`haka.dissector.FlowDissector` |nbsp|

    State-full dissector for TCP. It will associate each TCP packet with its matching connection
    and will build a stream from all received TCP packets.

    .. haka:attribute:: TcpConnectionDissector:srcip
                        TcpConnectionDissector:dstip

        :type: :haka:class:`ipv4.addr` |nbsp|

        Connection IP informations.

    .. haka:attribute:: TcpConnectionDissector:srcport
                        TcpConnectionDissector:dstport

        :type: number

        Connection port informations.

    .. haka:method:: TcpConnectionDissector:send([direction])

        :param direction: Direction to use (``'up'``, ``'down'`` or ``nil``)
        :paramtype direction: string

        Send the data that are ready in the streams.

    .. haka:method:: TcpConnectionDissector:drop()

        Drop the TCP connection. All future packets that belong to this connection will be
        silently dropped.

    .. haka:method:: TcpConnectionDissector:reset()

        Reset the TCP connection. A RST packet will be sent to both end and all future packet
        that belong to this connection will be silently dropped.

    .. haka:method:: TcpConnectionDissector:halfreset()

        Reset the TCP connection. A RST packet will be only sent to the server and all future packet
        that belong to this connection will be silently dropped.


Events
------

.. haka:function:: tcp_connection.events.new_connection(flow, tcp)
    :module:
    :objtype: event

    :param flow: TCP flow.
    :paramtype flow: :haka:class:`TcpConnectionDissector`
    :param tcp: TCP packet.
    :paramtype tcp: :haka:class:`TcpDissector`

    Event triggered whenever a new TCP connection is about to be created.

.. haka:function:: tcp_connection.events.end_connection(flow)
    :module:
    :objtype: event

    :param flow: TCP flow.
    :paramtype flow: :haka:class:`TcpConnectionDissector`

    Event triggered whenever a new TCP connection is being closed.

.. haka:function:: tcp_connection.events.receive_data(flow, current, direction)
    :module:
    :objtype: event


    **Event options:**

        .. haka:data:: streamed
            :noindex:
            :module:

            :type: boolean

            Run the event listener as a streamed function.

    :param flow: TCP flow.
    :paramtype flow: :haka:class:`TcpConnectionDissector`
    :param current: Current position in the stream. Will be a
        :haka:class:`vbuffer_iterator` if streamed option is on,
        :haka:class:`vbuffer_sub` otherwise.
    :paramtype current: :haka:class:`vbuffer_iterator` or
        :haka:class:`vbuffer_sub`
    :param direction: Data direction (``'up'`` or ``'down'``).
    :paramtype direction: string

    Event triggered when some data are available on a TCP stream.

    **Usage:**

    ::

        haka.rule{
            hook = tcp.events.receive_data,
            options = { streamed = true },
            eval = function(flow, current, direction)
                -- do some check and reaction
            end
        }

Helper
------

.. haka:module:: tcp_connection.helper

.. haka:class:: TcpFlowDissector
    :objtype: dissector

    .. haka:function:: TcpFlowDissector.dissect(cls, flow)

        :param cls: Current dissector class.
        :param flow: Parent Tcp flow.
        :ptype flow: :haka:class:`TcpConnectionDissector`

        Enable the dissector on a given flow.

    .. haka:function:: TcpFlowDissector.install_udp_rule(cls, port)

        :param cls: Current dissector class.
        :param port: Tcp port to select.
        :ptype port: number

        Create a security rule to enable the dissector on a given flow.

    .. haka:attribute:: TcpFlowDissector:__init(flow)

        :param flow: Parent Tcp flow.
        :ptype flow: :haka:class:`TcpConnectionDissector`

    .. haka:attribute:: TcpFlowDissector:flow

        :type: :haka:class:`TcpConnectionDissector` |nbsp|

        Underlying Tcp stream.

    .. haka:method:: TcpFlowDissector:reset()

        Reset the underlying Tcp connection.

    .. haka:method:: TcpFlowDissector:receive_streamed(iter, direction)

        :param iter: Current position in the stream.
        :paramtype iter: :haka:class:`vbuffer_iterator`
        :param direction: Data direction (``'up'`` or ``'down'``).
        :paramtype direction: string

        Function called in streamed mode that by default update the state machine
        instance. It can be overridden if needed.


Example
-------

.. literalinclude:: ../../../../sample/ruleset/tcp/rules.lua
    :language: lua
    :tab-width: 4

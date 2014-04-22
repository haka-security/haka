.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

TCP Connection
==============

.. haka:module:: tcp-connection

.. haka:class:: TcpConnectionDissector
    :module:
    :objtype: dissector

    :Name: ``'tcp-connection'``
    :Extend: :haka:class:`FlowDissector` |nbsp|

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

.. haka:function:: tcp-connection.events.new_connection(flow, tcp)
    :module:
    :objtype: event
    
    :param flow: TCP flow.
    :paramtype flow: :haka:class:`TcpConnectionDissector`
    :param tcp: TCP packet.
    :paramtype tcp: :haka:class:`TcpDissector`
    
    Event triggered whenever a new TCP connection is about to be created.

.. haka:function:: tcp-connection.events.end_connection(flow)
    :module:
    :objtype: event
    
    :param flow: TCP flow.
    :paramtype flow: :haka:class:`TcpConnectionDissector`
    
    Event triggered whenever a new TCP connection is being closed.

.. haka:function:: tcp-connection.events.receive_data(flow, stream, current, direction)
    :module:
    :objtype: event
    
    :param flow: TCP flow.
    :paramtype flow: :haka:class:`TcpConnectionDissector`
    :param stream: TCP data stream.
    :paramtype stream: :haka:class:`vbuffer_stream`
    :param current: Current position in the stream.
    :paramtype current: :haka:class:`vbuffer_iterator`
    :param direction: Data direction (``'up'`` or ``'down'``).
    :paramtype direction: string
    
    Event triggered when some data are available on a TCP stream.
    
    **Event options:**
    
        .. haka:data:: streamed
            :noindex:
            :module:
            
            :type: boolean
            
            Run the event listener as a streamed function.


Example
-------

.. literalinclude:: ../../../../sample/ruleset/tcp/rules.lua
    :language: lua
    :tab-width: 4


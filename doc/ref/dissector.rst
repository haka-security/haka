.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Dissector
=========

.. haka:module:: haka.dissector

.. haka:function:: new{...} -> dissector

    :param name: Dissector name.
    :paramtype name: string
    :param type: Dissector type
    :paramtype type: :haka:class:`Dissector`
    :return dissector: Created dissector. This object is a class that can be extended to implements the needed
        functions and properties.
    :rtype dissector: :haka:class:`class.Class`

    Create a new dissector.

.. haka:class:: Dissector
    :module:

    Dissector object.
    
    .. haka:function:: Dissector:register_event(name, continue[, signal [, options]])
    
        :param name: Name of the event.
        :paramtype name: string
        :param continue: Continuation function.
        :paramtype continue: function
        :param signal: Signaling function.
        :paramtype signal: function
        :param options: List of options.
        :paramtype options: table
        
        Register a new event for the dissector.
        
        .. haka:function:: continue(self) -> valid
            :module:
            :noindex:
        
            :param self: Current dissecteur.
            :paramtype self: :haka:class:`Dissector`
            :return valid: Should the event trigger continue.
            :rtype valid: boolean
            
            This function tests if the other listener on this events should be
            evaluated.
            
        .. haka:function:: signal(f, options, ...)
            :module:
            :noindex:
            
            :param f: Listener function.
            :paramtype f: function
            :param options: List of options from the event.
            :paramtype options: table
            :param ...: Extra parameters that should be passed to the listener.
            
            Signaling function used when a listener need to be called.

    .. haka:attribute:: Dissector.name
    
        :type: string
        
        Name of the dissector.
        
    .. haka:method:: Dissector:trigger(event, ...)
    
        :param event: Event to trigger.
        :paramtype event: :haka:class:`Event`
        :param ...: Parameters to pass to the event.
        
        Trigger an event.
        
    .. haka:method:: Dissector:drop()
        :abstract:
    
        Drop the dissector instance. It can be a packet or an flow depending on the
        dssector type.

    .. haka:method:: Dissector:error()
        :abstract:
        
        Called whenever an error is raised when inside the context of this dissector. The default
        implementation will do a :haka:func:`drop()`.

    .. haka:method:: Dissector:next_dissector()
        :abstract:
        
        Get the next dissector to use.


Utilities
---------

.. haka:function:: get(name) -> dissector

    :param name: Dissector name.
    :paramtype name: string
    
    Get a registered dissector by name.

.. haka:function:: pcall(dissector, f)

    :param dissector: Dissector to preotect.
    :paramtype dissector: :haka:class:`Dissector`
    :param f: Function to call.
    :paramtype f: function
    
    Protected call for a function inside a dissector context.

.. haka:function:: other_direction(dir) -> other_dir

    :param dir: Direction ``'up'`` or ``'down'``.
    :paramtype dir: string
    :return other_dir: Other direction.
    :rtype other_dir: string
    
    Utility function to get the other direction.


Dissector types
---------------

Packet
^^^^^^

.. haka:class:: PacketDissector
    :objtype: dissector

    :extend: :haka:class:`Dissector` |nbsp|
    
    Basic packet dissector.
    
    .. haka:method:: PacketDissector.receive_packet(pkt)
        :module:
        :objtype: event
        
        :param pkt: Packet representation.
        :paramtype pkt: :haka:class:`PacketDissector`
        
        Event that is triggered whenever a new packet is received.
    
    .. haka:method:: PacketDissector.send_packet(pkt)
        :module:
        :objtype: event
        
        :param pkt: Packet representation.
        :paramtype pkt: :haka:class:`PacketDissector`
        
        Event that is triggered just before sending the packet to the upper layer.
        
    .. haka:function:: PacketDissector.receive(prev)
    
        :param prev: Previous dissector object.
        :paramtype prev: :haka:class:`Dissector`
        
        Function called to dissect a packet from data comming from another dissector.
        
    .. haka:method:: PacketDissector:continue()
        :abstract:
    
        Function that abort if the packet should no longer be processed.

    .. haka:method:: PacketDissector:send()
        :abstract:
        
        Send the packet.

    .. haka:method:: PacketDissector:inject()
        :abstract:
        
        Inject the packet.

Encapsulated packet
^^^^^^^^^^^^^^^^^^^

.. haka:class:: EncapsulatedPacketDissector
    :objtype: dissector

    :extend: :haka:class:`PacketDissector` |nbsp|

    Packet dissector that is build above another packet payload (ICMP over IP for instance).

    .. haka:method:: EncapsulatedPacketDissector:parse_payload(pkt, payload)
        :abstract:
        
        :param pkt: Parent dissector packet.
        :paramtype pkt: :haka:class:`Dissector`
        :param payload: Payload to be parsed by this dissector.
        :paramtype payload: :haka:class:`vbuffer`
    
        Parse the payload coming from the previous dissector packet.

    .. haka:method:: EncapsulatedPacketDissector:create_payload(pkt, payload, init)
        :abstract:
        
        :param pkt: Parent dissector packet.
        :paramtype pkt: :haka:class:`Dissector`
        :param payload: Payload to be parsed by this dissector.
        :paramtype payload: :haka:class:`vbuffer`
        :param init: Initialization field for the packet.
        
        Build a new payload.

    .. haka:method:: EncapsulatedPacketDissector:forge_payload(pkt, payload)
        :abstract:
        
        :param pkt: Parent dissector packet.
        :paramtype pkt: :haka:class:`Dissector`
        :param payload: Payload to be parsed by this dissector.
        :paramtype payload: :haka:class:`vbuffer`
        
        Called when the packet is about to be send.

Flow
^^^^

.. haka:class:: FlowDissector
    :objtype: dissector

    :extend: :haka:class:`Dissector` |nbsp|

    Dissector for a flow (multiple packets). An example is HTTP for instance.

    .. haka:data:: FlowDissector.connections

        :type: table
        
        Table of connections to instanciate when the dissector is created.

    .. haka:method:: FlowDissector:streamed(f, stream, current, ...)
        
        :param f: Function to execute.
        :paramtype f: function
        :param stream: Stream.
        :paramtype stream: :haka:class:`vbuffer_stream`
        :param current: Current position in the stream.
        :paramtype current: :haka:class:`vbuffer_iterator`
        :param ...: Parameters that are given to *f*.
        
        Execute a function in streamed mode. In this mode, Haka will use coroutine to
        execute it in a thread like environement. It allows the function to block waiting
        for available data on the stream.
        
        This function is manly used as the signal function for event based on stream.
            
        **Example:**
        
        ::
        
            HttpDissector:register_event('request_data', nil, haka.dissector.FlowDissector.stream_wrapper)

    .. haka:method:: FlowDissector:get_comanager(stream) -> manager

        :param stream: Stream used as the key.
        :paramtype stream: :haka:class:`vbuffer_stream`
        :return manager:
        :rtype manager:
        
        Retreived the stream coroutine manager for a given stream.

Examples
^^^^^^^^

For dissector examples, check the supported :doc:`hakadissector`.
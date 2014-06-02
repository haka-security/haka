.. _smtp_dissector:

Dissector
---------
First of all, we load the required packages and initialize the dissector module

.. code-block:: lua
    
    local class = require('class')
    local tcp_connection = require('protocol/tcp_conenction')

    local module = {}

Creating the dissector
^^^^^^^^^^^^^^^^^^^^^^
Next, we create the dissector by specifying its name and its type:

.. code-block:: lua

    local smtp_dissector = haka.dissector.new{
        type = haka.dissector.FlowDissector,
        name = 'smtp'
    }

Initializing the dissector
^^^^^^^^^^^^^^^^^^^^^^^^^^
The created dissector, namley `smtp-dissector`, is a particular class. Before instanciating it, we need to define first its constructor:

.. code-block:: lua

    function smtp_dissector.method:__init(flow)
        class.super(smtp_dissector).__init(self)
        self.flow = flow
        self.state = smtp_dissector.states:instanciate(self)
    end

This constructor function takes as input a flow passed by the previous dissector and instanciates the state machine (see section :ref:`smtp_state_machine`).

Selecting SMTP dissector
^^^^^^^^^^^^^^^^^^^^^^^^
At this point, the dissector is created but not yet instaciated. Here, we define two rules which are made available on the dissector module. The former is in fact a security rule which selects, at connection establishement, the dissector to use. The latter, instanciates the dissector.

.. code-block:: lua

    function module.install_tcp_rule(port)
        haka.rule{
            hook = tcp_connection.events.new_connection,
            eval = function (flow, pkt)
                if pkt.dstport == port then
                    haka.log.debug('smtp', "selecting smtp dissector on flow")
                    module.dissect(flow)
                end 
            end 
        }   
    end

    function module.dissect(flow)
        flow:select_next_dissector(smtp_dissector:new(flow))
    end

Managing data reception
^^^^^^^^^^^^^^^^^^^^^^^
Each flow-dissector must implement a `receive` method. In the case of application protocols, this function has three parameters: the stream, the current position in the stream and the direction (`up` or `down`).

The `receive` method is called by the previous dissector whenever new data is available which in turns calls the `streamed_receive` function in streamed mode. That is, the function will block waiting for available data on the stream to proceed. Data that are ready on the stream are then sent. 

Note that the `receive` core function is executed in a protected environment which enable
us to catch errors.

.. code-block:: lua

    function smtp_dissector.method:receive(stream, current, direction)
        return haka.dissector.pcall(self, function ()
            if not self._receive_callback then
                self._receive_callback = haka.event.method(self, smtp_dissector.method.receive_streamed)
            end
            self.flow:streamed(self._receive_callback, stream, current, direction)
            if self.flow then
                self.flow:send(direction)
            end
        end)
    end

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

.. _smtp_dissector:

The following function passes the control to the current state of the
state-machine to handle new available data and then checks, through a call to
`continue` function, if stream processing should be aborted (e.g. dropping
connection) or not.

.. code-block:: lua

    function smtp_dissector.method:receive_streamed(flow, iter, direction)
        assert(flow == self.flow)
        while iter:wait() do
            self.states:update(direction, iter)
            self:continue()
        end
    end

Adding extras properties and functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As stated above, `smtp-dissector` is a paritcular class (see :doc:`\../../../ref/class`) which could be extended by
adding extras methods ans properties.

* Adding properties: this is done through the property field of smtp_dissector class. 

.. code-block:: lua

    smtp_dissector.property.connection = {
        get = function (self)
            self.connection = self.flow.connection
            return self.connection
        end
    }

* Adding methods: this is done through the `method` field of `smtp_dissector` class. In our example, we define the mandatory function `continue` and two extras functions `drop` and `reset` to drop and reset a smtp connection, respectively.

.. code-block:: lua

    function smtp_dissector.method:continue()
        if not self.flow then
            haka.abort()
        end
    end

    function smtp_dissector.method:drop()
        self.flow:drop()
        self.flow = nil
    end

    function smtp_dissector.method:reset()
        self.flow:reset()
        self.flow = nil
    end

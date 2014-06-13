.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Loading required modules
^^^^^^^^^^^^^^^^^^^^^^^^
In order to create a smtp dissector, we create a new file (`smtp.lua`) and save it in a
location that will be used later as a path for all Haka scripts requiring this
dissector.

The first step it to load the required packages and initialize the dissector module:

.. code-block:: lua

    local class = require('class')
    local tcp_connection = require('protocol/tcp_connection')

    local module = {}

Note that we load the `tcp_connection` module since we built a smtp dissector over
tcp protocol.

Creating the dissector
^^^^^^^^^^^^^^^^^^^^^^
Next, we create the dissector by specifying its name and its type:

.. code-block:: lua

    local SmtpDissector = haka.dissector.new{
        type = haka.dissector.FlowDissector,
        name = 'smtp'
    }

We select a `FlowDissector` type since smtp communications are flow-based (i.e.
multiple packets are exchanged during a smtp session).

Initializing the dissector
^^^^^^^^^^^^^^^^^^^^^^^^^^
The created dissector, namely `SmtpDissector`, is a particular class. Before instanciating it, we need to define first its constructor:

.. code-block:: lua

    function SmtpDissector.method:__init(flow)
        class.super(SmtpDissector).__init(self)
        self.flow = flow
        self.state = SmtpDissector.states:instanciate(self)
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
        flow:select_next_dissector(SmtpDissector:new(flow))
    end

Managing data reception
^^^^^^^^^^^^^^^^^^^^^^^
Each flow-dissector must implement a `receive` method to manage data received from low-layer dissector. In the case of application protocols, this function has three parameters: the stream, the current position in the stream and the direction (`up` or `down`).

The `receive` method is called by the previous dissector (`tcp_connection`) whenever new data is available which in turns calls the `streamed_receive` function. This function is executed in a streamed mode. That is, the function will block waiting for available data on the stream to proceed. Data that are ready on the stream are then sent.

Note that the `receive` function is executed in a protected environment which enable
us to catch errors and correctly handle them. A method error is called in this case on the dissector. By default, it will drop the connection and this it is not needed to redefine it.

.. code-block:: lua

    function SmtpDissector.method:receive(stream, current, direction)
	    return haka.dissector.pcall(self, function ()
		    self.flow:streamed(stream, self.receive_streamed, self, current, direction)

    		if self.flow then
	    		self.flow:send(direction)
		    end
    	end)
    end

.. _SmtpDissector:

The following function passes the control to the current state of the
state-machine to handle new available data and then checks, through a call to
`continue` function, if stream processing should be aborted (e.g. dropping
connection) or not.

.. code-block:: lua

    function SmtpDissector.method:receive_streamed(iter, direction)
    	while iter:wait() do
	    	self.state:transition('update', direction, iter)
	    	self:continue()
    	end
    end

Adding extras properties and functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As stated above, `SmtpDissector` is a paritcular class (see :doc:`\../../../ref/class`) on which you can add extra methods and properties.

* Adding properties: this is done through the property field of SmtpDissector class.

.. code-block:: lua

    SmtpDissector.property.connection = {
        get = function (self)
            self.connection = self.flow.connection
            return self.connection
        end
    }

* Adding methods: this is done through the `method` field of `SmtpDissector` class. In our example, we define the mandatory function `continue` and two extras functions `drop` and `reset` to drop and reset a smtp connection, respectively.

.. code-block:: lua

    function SmtpDissector.method:continue()
        if not self.flow then
            haka.abort()
        end
    end

    function SmtpDissector.method:drop()
        self.flow:drop()
        self.flow = nil
    end

    function SmtpDissector.method:reset()
        self.flow:reset()
        self.flow = nil
    end

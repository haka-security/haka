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
        type = tcp_connection.helper.TcpFlowDissector,
        name = 'smtp'
    }

We select a :haka:class:`tcp_connection.helper.TcpFlowDissector` type since smtp communications are over
Tcp and flow-based (i.e. multiple packets are exchanged during a smtp session).

Initializing the dissector
^^^^^^^^^^^^^^^^^^^^^^^^^^
The created dissector, namely `SmtpDissector`, is a particular class.

Before instanciating it, we could define a constructor if needed:

.. code-block:: lua

    function SmtpDissector.method:__init(flow)
        class.super(SmtpDissector).__init(self, flow)
        [...]
    end

This constructor function pass its input flow to the dissector type. It will
also automatically instanciate our state machine.

Selecting SMTP dissector
^^^^^^^^^^^^^^^^^^^^^^^^
At this point, the dissector is created but not yet instaciated. Here, we
define two rules which are made available on the dissector module. The former
is in fact a security rule which selects, at connection establishment, the
dissector to use. The latter, allow a user rule to activate our dissector.

.. code-block:: lua

    module.install_tcp_rule = SmtpDissector:install_tcp_rule()
    module.dissect = SmtpDissector:dissect()

.. _SmtpDissector:

Adding extras properties and functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As stated above, `SmtpDissector` is a particular class (see :doc:`\../../../ref/class`)
on which you can add extra methods and properties. You can refer to this section to get
details about how this could be achieved.

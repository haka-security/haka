.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Events
------
Events are the glue between dissectors and security rules. Dissectors create
events and then trigger them. As a result, all security rules hooking on that
events will be evaluated.


Registering events
^^^^^^^^^^^^^^^^^^
Events are created by invoking `register_event` method wich takes as first argument an event name. For our needs, we will create three events:

.. code-block:: lua

    smtp_dissector:register_event('command')
    smtp_dissector:register_event('response')
    smtp_dissector:register_event('mail_content', nil, haka.dissector.FlowDissector.stream_wrapper)


Triggering events
^^^^^^^^^^^^^^^^^
Events are triggered by invoking `trigger` method which is usually done after successfully parsing a message block. We pass to the trigger méthod the event name and a list of parameters that will beavailable later to the security rule through `eval`'s arguments. For instance, the following call is made while parsing a smtp command in the state machine:

.. code-block:: lua

    self:trigger('command', self.smtp)

.. note:: `self` is an instance of smtp dissector.


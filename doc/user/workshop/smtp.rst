.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

SMTP protocol dissection
========================
This tutorial covers the specification of smtp protocol using Haka. For those who are not
familiar with this protocol, please refer to :ref:`smtp_appendix`.

In smtp protcol, we distinguish three smtp messages: *commands*, *responses* and *data* (mail
content). We provide here a partial specification :download:`smtp.lua` that manages
only smtp commands and mail data. Your overall goal is to complete this specification by
managing also smtp responses.

Grammar
-------

Haka features a `grammar <../../ref/grammar.html>`_ allowing to specify
protocol message synatx. Use this grammar to complete the partial specification
with smtp responses support:

A smtp response message is defined as:

    * a status code followed by a separator ("- ")
    * a comprehensive message
    * a trailing CRLF

.. admonition:: Exercise

    Given the following skeleton, write the missing grammar to complete our
    dissector.

    .. code-block:: lua

        smtp_response = record{
            CODE,
            SEP,
            MESSAGE,
            CRLF
        }

    Test your configuration. We provide a sample security file
    :download:`smtp_test.lua` that loads the partial configuration script and
    activates the dissector on port 25:

    * open a new window and start haka:

        .. code-block:: console

            haka -c haka.conf -r smtp_test.lua --no-daemon

    * open a new window and connect to the smtp server

        .. code-block:: console

            nc 127.0.0.1 25

    .. note::

        haka will warn that some states are never reached which is
        expected result since we haven't defined yet a `response` state in the state
        machine.

    You can also use a pcap file that contains an smtp transaction: :download:`smtp.pcap`.

    .. tip::

         You can also the use the ``-d`` option to ``haka`` or ``hakapcap`` to get a lot
         of debugging messages for the grammar parsing and the state machine execution.

Smtp server may respond with a sequence of response messages. You can rely on the
`array` grammar entity to represent a list of smtp responses.

.. admonition:: Exercise

    Complete your dissector to support sequence of response messages.

    .. code-block:: lua

        smtp_responses = field('responses',
            array(...)
                :untilcond(function (elem, ctx)
                    [...]
                end)
            )

    .. note:: `untilcond` is an array option that returns true to indicate that
        we have reached the end of the array.

    Test you new grammar.

.. seealso:: `Grammar <../../ref/grammar.html>`_ full documentation.

.. _smtp_events:

Events
------
Events are the glue between dissectors and security rules. Dissectors create
events and then trigger them. As a result, all security rules hooking to that
events will be evaluated. Non stream-based events are created by invoking
``register_event()`` method which takes an event name as first argument.

.. code-block:: lua

    SmtpDissector:register_event(<name>)

.. admonition:: Exercise

    Update the partial specification by creating an event named ``response``.

We are going to trigger this event later inside the state machine code.

State machine
-------------
Smtp `state machine <../../ref/state_machine.html>`_ is created through
the following skeleton. The first step is to set the type of the states. In our
case, we select a bidirectional type in order to handle data parsing in both
direction: *up* (from client to server) and *down* (from server to client).
Then, we create the required states and transitions between states. Finally, we
select the initial state.

.. code-block:: lua

    SmtpDissector.states = haka.state_machine.new("smtp", function ()
        -- setting the type of the states
        state_type(BidirectionalState)

        [...]

        initial(<first state>)
    end)

The partial specification :download:`smtp.lua` defines four steps to handle
*session initiation*, *client_initiation*, *commands*, and *data transmission*.
Your goal is to complete this specification by adding a new state *response*
along with his transitions.

In a bidirectional setting, we create a new state by passing the expected
compiled grammar for each direction (``'up'`` and ``'down'``):

.. admonition:: Exercise

    Create a new state ``response``.

Next step is to set the transition for this state. A transition is created
through the following skeleton. A transition consists of:

    * ``event``: an event to attach to. Do not confuse with user defined
      dissector event. Those are built-in events specific to the state machine type.

    * ``when``: a checking function that takes the decision if we should switch to
      another state and/or perform a specific action. By default (i.e. missing
      when function), the action is taken and the jump is followed.

    * ``execute``: an action to perform.

    * ``jump``: a state to jump to.

.. code-block:: lua

    some_state:on{
        event = ...,
        when = function (self, res) ... end,
        execute = function (self, res) ... end,
        jump = another_state,
    }

.. admonition:: Exercise

    Create a first transition on ``response`` attached to
    ``events.down`` event (remember that we are expecting data from server to
    client) that jump to state ``data_transmission`` when the state code is
    equal to 354. Parsing results are available through the ``res`` variable.

    .. note:: Do not forget to trigger the ``response`` event in the ``execute``
        reserved field.

.. admonition:: Exercise

    Similarly, create a second transition on `response` attached to
    ``events.down`` event that switch to a termination state (`jump = finish`)
    if the status code is equal to 221.

.. admonition:: Exercise

    Create a third transition that switch to ``command`` state if the above
    conditions are not met.

There is one more case to handle, if the parsing of the grammar fails, this need to be
reported.

.. admonition:: Exercise

    Define a transition attached to ``events.parse_error`` to report errors when
    response messages do not comply to their specification.

.. admonition:: Exercise

    Now, we are ready to test the whole specification:

    * Start haka

        .. code-block:: console

            haka -c haka.conf -r smtp_test.lua --no-daemon

    * Start a transaction mail

        .. code-block:: console

            nc 127.0.0.1 25
            HELO <some_domain>
            ...

    .. tip:: you can update your specification by adding debug output in order to dump the parsing result:

        .. code-block:: lua

            debug.pprint(some_table)

Security rules
--------------
The purpose of these security rules is to show how to use the previously defined
events (:ref:`smtp_events`) and how to filter smtp packets based on fields
extracted from parsing results.

Filtering spam
^^^^^^^^^^^^^^
In order to filter spam we create a security rule that hooks to the ``command``
event in order to filter the banned domain 'suspicious.org'. We can react to this
by raising an alert and by dropping the connection.

.. admonition:: Exercise

    Create this security rule and test it against :download:`smtp.pcap<smtp.pcap>`.

    .. code-block:: console

        $ hakapcap spam_filter.lua smtp.pcap

    .. tip:: Load the ``regexp/pcre`` pattern matching engine to check the parameter
        of the ``mail`` command.

You can get the full code here :download:`spam_filter.lua`.

Dump mail content
^^^^^^^^^^^^^^^^^
It is also possible to dump the content of the mail. To do so, we create a
second security rule that hooks to the ``mail_content`` event.

.. code-block:: lua

    haka.rule{
        hook = ...,
        options = {
            streamed = true,
        },
        eval = function (flow, iter)
            [...]
        end
    }

.. admonition:: Exercise

    Create this security rule and test it against :download:`smtp.pcap`.

    .. code-block:: console

        $ hakapcap mail_content.lua smtp.pcap

We provide the full code of the above script here :download:`mail_content.lua`.

Full dissector
--------------

The full dissector code can be downloaded here: :download:`smtp_final.lua`

It is possible to generate the graph related to the state-machine and the
grammar (you need graphviz for this):

.. code-block:: console

    $ hakapcap smtp_test.lua smtp.pcap --dump-dissector-graph
    $ dot -Tpdf -o smtp.pdf smtp-state-machine.dot

.. _smtp_appendix:

Appendix
--------

SMTP protocol
^^^^^^^^^^^^^
SMTP stands for Simple Mail Transfer Protocol and was designed to deliver mail
reliability. It is a command/reponses protocol that starts with a session initiation
during which the server sends first a welcoming message together with a status
code indicating if the transaction has succeed (2XX) or not (5XX). Then, the
client identifies himself using the commands EHLO or HELO and awaits for server
response to proceed::

    S: 220 foo.com Simple Mail Transfer Service Ready
    C: EHLO bar.com
    S: 250-foo.com greets bar.com
    S: 250-8BITMIME
    S: 250-SIZE
    S: 250-DSN

Mail transaction starts after the above initiation phase. At each step, the
client sends a smtp command and receives one or multiple response messages
from the server. At least three steps are required to transfer a mail, namely, through
MAIL, RCPT and DATA commands::

    C: MAIL FROM:<Smith@bar.com>
    S: 250 OK
    C: RCPT TO:<Jones@foo.com>
    S: 250 OK
    C: DATA
    S: 354 Start mail input; end with <CRLF>.<CRLF>
    C: some data...
    C: ...etc. etc. etc.
    C: .
    S: 250 OK

Finally, the connection ends with a QUIT message::

    C: QUIT
    S: 221 foo.com Service closing transmission channel


.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Creating the state machine
^^^^^^^^^^^^^^^^^^^^^^^^^^
Smtp state machine is created through the following skeleton. The first step is
to set the type of the states. In our case, we select a bidirectionnal type (
See :doc:`\../../../ref/state_machine`) in order to handle data parsing in both
direction: `up` (from client to server) and `down` (from server to client).
Then, we create the required states and transitions between states. Finally, we
select the initial state.

.. code-block:: lua

    SmtpDissector.states = haka.state_machine.new("smtp", function ()
        -- setting the type of the states
        state_type(BidirectionalState)

        -- creating new states
        state_1 = state(...)
        state_2 = state(...)

        -- creting a transition for state_1
        state_1:on{
            ...
        }

        -- creating a second transition for state_1
        state_1:on{
            ...
        }

        -- creating a transition for state_2
        state_2:on{
            ...
        }

        -- setting common transitions
        any:on{
            ...
        }

        -- setting the initial state
        initial(state_1)
    end)

Creating new states
^^^^^^^^^^^^^^^^^^^
In a bidiretionnal setting, we create a new state by passing the expected
compiled grammar for each direction. In the following, we create five states to
manage initiation phase, command/response and data transfert:

.. code-block:: lua

    session_initiation = state(nil, SmtpDissector.grammar.smtp_responses)
    client_initiation = state(SmtpDissector.grammar.smtp_command, nil)
    response = state(nil, SmtpDissector.grammar.smtp_responses)
    command = state(SmtpDissector.grammar.smtp_command, nil)
    data_transmission = state(SmtpDissector.grammar.smtp_data, nil)

.. note:: Note that we provide no grammar when messages are not expected in a given direction. If this happens (e.g. command received from client while expecting a response from server), a special event `missing_grammmr` is triggered to handle this error.


Creating transitions
^^^^^^^^^^^^^^^^^^^^
A transition is created through the following skeleton:

.. code-block:: lua

    some_state:on{
        event = ...,
        when = function (...) ... end,
        execute = function (...) ... end,
        jump = another_state,
    }

A transition consists of:
 * *event*: an event to attach to. Do not confuse with user defined event. They
   are built-in events specific to the state machine type.
 * *when*: a checking function that takes the decision if we should switch to another state
   and/or to perform a specific action. By default (i.e. missing check function), the `execute`
   is performed and the `jump` is followed.
 * *execute*: an action to perform.
 * *jump*: the state to switch to.

Managing initiation phase
~~~~~~~~~~~~~~~~~~~~~~~~~
This phase consists of two steps. In the first step, the client waits for a welcoming message. This is handled by our initial state `session_initiation`.

We define transitions on the `down` event since we are expecting data from the server. These transitions are evaluated in the order in which they are defined.

In the first transition, we trigger a `response` event and switch to the
`client_initiation` state if the status response code is equal to '220'.
Otherwise, the second transition is evaluated. In this case, we report an
alert and switch to a built-in failure state.

.. code-block:: lua

    session_initiation:on{
        event = events.down,
        when = function (self, res) return res.responses[1].code == '220' end,
        execute = function (self, res)
            self:trigger('response', res)
        end,
        jump = client_initiation,
    }

    session_initiation:on{
        event = events.down,
        execute = function (self, res)
            haka.alert{
                description = string.format("unavailable service: %s", status),
                severity = 'low'
            }
        end,
        jump = fail,
    }

We define also a transition on `parse_error` event to report error when smtp responses do not comply to their specification.

.. code-block:: lua

    session_initiation:on{
        event = events.parse_error,
        execute = function (self, err)
            haka.alert{
                description = string.format("invalid smtp response %s", err),
                severity = 'high'
            }
        end,
        jump = fail,
    }

In the same way, we define `client_initiation` transitions attaching this time to the `up` event since we are expecting only messages from the client.

In the first transition, we check that the `command` value (this value is
available in the parsing result `res`; remember that we defined a `field` named
command in our grammar) is equal to 'HELO' or 'EHLO'. If this condition is
satisfied, we store the parsing result and made it available to security rules
attached to the triggered event `command` and then jump to `response` state.
Otherwise, we jump to a failure state.

.. code-block:: lua

    client_initiation:on{
        event = events.up,
        when = function (self, res)
            local command = string.upper(res.command)
            return command == 'EHLO' or command == 'HELO'
        end,
        execute = function (self, res)
            self.command = res
            self:trigger('command', res)
        end,
        jump = response,
    }

    client_initiation:on{
        event = events.up,
        execute = function (self, res)
            haka.alert{
                description = string.format("invalid client initiation command"),
                severity = 'low'
            }
        end,
        jump = fail,
    }

Similarly, we attach a transition on `parse_error` event that will report an error in case of unexpected smtp command.

.. code-block:: lua

    client_initiation:on{
        event = events.parse_error,
        execute = function (self, err)
            haka.alert{
                description = string.format("invalid smtp command %s", err),
                severity = 'low'
            }
        end,
        jump = fail,
    }

Managing command/response comunication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Two states have been defined previously to manage command and response. In the former,
we expect messages from `up` direction to conform to the `smtp-command` grammar.
In the latter, we expect message from `down` direction to conform to the
`smtp-responses` grammar and if messages are well-formed then we move to the
approriate state by checking the status code.

.. code-block:: lua

    response:on{
        event = events.down,
        when = function (self, res)
            return res.responses[1].code == '354'
        end,
        execute = function (self, res)
            self.response = res
            self:trigger('response', res)
        end,
        jump = data_transmission,
    }

    response:on{
        event = events.down,
        when = function (self, res)
            return res.responses[1].code == '221'
        end,
        execute = function (self, res)
            self.response = res
            self:trigger('response', res)
        end,
        jump = finish,
    }

    response:on{
        event = events.down,
        execute = function (self, res)
            self.response = res
            self:trigger('response', res)
        end,
        jump = command,
    }


And as usual, we move to a failure state in case of parsing errors:

.. code-block:: lua

    response:on{
        event = events.parse_error,
        execute = function (self, err)
            haka.alert{
                description = string.format("invalid smtp response %s", err),
                severity = 'low'
            }
        end,
        jump = fail,
    }

.. note:: Have a look at :download:`smtp.lua<../../../../sample/smtp_dissector/smtp.lua>` to get the full code of the transitions defined on `command` state.

Managing content mail transfert
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
We get here (i.e. `data_transmission` state) if server responds with a status code 354 to a DATA command.

First of all, we define a transition attached to `enter` event to build stream in order to collect mail content:

.. code-block:: lua

    data_transmission:on{
        event = events.enter,
        execute = function (self)
            self.mail = haka.vbuffer_sub_stream()
        end,
    }

Next, we want to send the mail data pieces by pieces as soon as available to the
security rules. To do this we add a callback on the grammar for the data:

.. code-block:: lua

    smtp_data = record{
        field('data', bytes()
            :untiltoken("%r?%n%.%r?%n")
            :chunked(function (self, sub, last, ctx)
                ctx.user:push_data(sub, last)
            end)),
        token("%r?%n%.%r?%n")
    }

The ``chunked`` callback allow to push data into a streamed view. The code of this function
is available in the full smtp code.

We just have to add a simple transition to go back to the ``response`` state when the
data transfer is over.

.. code-block:: lua

    data_transmission:on{
        event = events.up,
        jump = response,
    }

Finally, we destroy the stream while leaving the `data_transmission` state:

.. code-block:: lua

    data_transmission:on{
        event = events.leave,
        execute = function (self)
            self.mail = nil
        end,
    }

Additionnaly, a transition is defined to handle parsing errors:

.. code-block:: lua

    data_transmission:on{
        event = events.parse_error,
        execute = function (self, err)
            haka.alert{
                description = string.format("invalid data blob %s", err),
                severity = 'low'
            }
        end,
        jump = fail,
    }

Setting default transitions
^^^^^^^^^^^^^^^^^^^^^^^^^^^
We define two transitions that are common to all states. The first one is used
to handle errors which we manage by dropping the connection:

.. code-block:: lua

    any:on{
        event = events.fail,
        action = function (self)
            self:drop()
        end,
    }

The second transition allows to handle the cases where messages are not expected from client or server.

.. code-block:: lua

    any:on{
        event = events.missing_grammar,
        execute = function (self, direction, payload)
            local description
            if direction == 'up' then
                description = "unexpected client command"
            else
                description = "unexpected server response"
            end
            haka.alert{
                description = description,
                severity = 'low'
            }
        end,
        jump = fail,
    }

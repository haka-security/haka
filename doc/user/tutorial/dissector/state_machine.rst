.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Creating the state machine
^^^^^^^^^^^^^^^^^^^^^^^^^^
Smtp state machine is created through the following skeleton where we define five states to manage intiation phase, command/response and data transfert. For each of these states we define two main transitions `up` and `down` that will handle received data depending on their direction: from client to server or from server to client.

.. code-block:: lua

    SmtpDissector.states = haka.state_machine("smtp", function ()

        session_initiation = state {
            up = function (...)

            down = function (...)
        }

        client_initiation = state {
               ...
        }

        respone = state {
            ...
        }

        command = state {
            ...
        }

        data_transfert = state {
            ...
        }

        default_transition{
            error = function (self)
                self:drop()
            end,
            update = function (self, direction, iter)
                return self.state:transition(direction, iter)
            end
        }

        initial(session_initiation)
    end)


`default_transitions` are transitions common to all states. In our case, all states share these two transitions:

* `error`: built-in transition used to handle errors which we manage by dropping the connection (remember that we defined a drop connection in our dissector).

* `update`: user-defined transition. It's only purpose it to pass the control to the rigth transition (`up̀` or `down`) when new data are available.

Finally, the last line allow us to select the initial state.

Adding new states
^^^^^^^^^^^^^^^^^

Managing initiation phase
~~~~~~~~~~~~~~~~~~~~~~~~~

This phase consists of two steps. In the first step, the client waits for a welcoming message. This is handled by our initial state `session_initiation`:

.. code-block:: lua

    session_initiation = state {
        up = function (self, iter)
            haka.alert{
                description = "unexpected client command",
                severity = 'low'
            }
            return 'error'
        end,
        down = function (self, iter)
            local err
            self.response, err = SmtpDissector.grammar.smtp_responses:parse(iter, nil, self)
            if err then
                haka.alert{
                    description = string.format("invalid smtp response %s", err),
                    severity = 'high'
                }
                return 'error'
            end
            local status = self.response.responses[1].code
            if status == '220' then
                self:trigger('response', self.response)
                return 'client_initiation'
            else
                haka.alert{
                    description = string.format("unavailable service: %s", status),
                    severity = 'low'
                }
                return 'error'
            end
        end
    }

In the `up` transition, we report an error as we are not expecting to receive command from client. In the `down` transition, we parse the received data and check if their syntax conform to the grammar defined previously for responses messages: `smtp_responses`, and report an error otherwise. If the received message is well-formed then we check its status code, trigger a `response` event, and switch to the `client_intiation` state.

.. note:: For a sake of convenience, we do not fully manage smtp status code. For instance, we must switch to a state where we handle the case where the service is unavailable instead of reporting  an error. Creating a new state to manage this case is left as an exercice to the reader.

In the same way, we define a `client_initiation` state where we report an error in the `down` transition and parse the received message in the `up` transtion. Note that we check additionnaly that the `command` value (this value is avalable in the parsing result ; remember that we defined a `field` named command in our grammar) must be equal to 'HELO' or 'EHLO':

.. code-block:: lua

    client_initiation = state {
        up = function (self, iter)
            local err
            self.command, err = SmtpDissector.grammar.smtp_command:parse(iter, nil, self)
            if err then
                haka.alert{
                    description = string.format("invalid smtp command %s", err),
                    severity = 'low'
                }
                return 'error'
            end
            local command = string.upper(self.command.command)
            if command == 'EHLO' or command == 'HELO' then
                self:trigger('command', self.command)
                return 'response'
            else
                haka.alert{
                    description = string.format("invalid client initiation command"),
                    severity = 'low'
                }
                return 'error'
            end
        end,
        down = function (self, iter)
            haka.alert{
                description = string.format("unexpected server response"),
                severity = 'low'
            }
            return 'error'
        end,
    }

We switch to response state in case of successful parsing.

Managing command/response comunication
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Simillarly, we define two states to manage command and response. In the former, we expect messages from `up` direction to conform to the `smtp-command` grammar. In the latter, we expect message from `down` direction to conform to the `smtp-responses` grammar and if messages are well-formed then we move to the approriate state by checking the status code:

.. code-block:: lua

    response = state {
        up = function (self, iter)
            ...
        end,
        down = function (self, iter)
            ...
            local status = self.response.responses[1].code
            if status == '354' then
                return 'data_transmission'
            elseif status == 221 then
                return 'finish'
            else
                return 'command'
            end
        end
    }

.. note:: Have a look at :download:`smtp.lua<smtp.lua>` to get the full code of response and command states.


Managing content mail transfert
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
We get here (i.e. `data_transmission` state) if server responds with a status code 354 to a DATA command:

.. code-block:: lua

    data_transmission = state {
        enter = function (self)
            self.mail = haka.vbuffer_sub_stream()
        end,
        up = function (self, iter)
            local data, err = SmtpDissector.grammar.smtp_data:parse(iter, nil, self)
            if err then
                haka.alert{
                    description = string.format("invalid data blob %s", err),
                    severity = 'low'
                }
                return 'error'
            end
            local end_data = data.data:asstring() == '.\r\n'
            local mail_iter = nil
            if end_data then
                self.mail:finish()
            else
                mail_iter = self.mail:push(data.data)
            end
            self:trigger('mail_content', self.mail, mail_iter)
            self.mail:pop()
            if end_data then
                return 'response'
            end
        end,
        down = function (self, iter)
            haka.alert{
                description = string.format("unexpected server response"),
                severity = 'low'
            }
            return 'error'
        end,
        leave = function (self)
            self.mail = nil
        end,
    }

The above state defines two predefined transitions `enter` and `leave` which are activated when entering an leaving the state, respectively. The former is used to build a stream to collect mail content whereas the latter is used to destroy the stream. We will focus here on the `up` transtion where the data are first parsed then pushed on the stream. If we detect an end of mail transfert (line made of a single '.' followed by a traling CRLF), then we mark that we reached the end of the stream and switch again to the `command` state where the client can issue a new transaction mail.

Note that thanks to the stream, we are able to collect mail content in a streamed
fashion by blocking transparently when data are not yet available.


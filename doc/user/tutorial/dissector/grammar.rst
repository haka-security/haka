.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua


.. _smtp_grammar:

Grammar
-------

Grammar building blocks
^^^^^^^^^^^^^^^^^^^^^^^
Haka grammar is made of basic blocks and coumpound blocks. The former enable parsing of basic elements such as booleans (`flag`), bytes (`bytes`), number (`number`), regular expression (`token`), etc. The latter allow to form complex blocks by combining basic and compound blocks. For instance, the `record` block is used to define a structure of elements.

Creating the grammar
^^^^^^^^^^^^^^^^^^^^
Smtp protocol grammar is defined following this squeleton where we define
protocol messages syntax and then export them. That is, compiled grammar blocks
(e.g. exported elements) are ready to be parsed:

.. code-block:: lua

    SmtpDissector.grammar = haka.grammar.new("smtp", function ()

        ....

        smtp_command = entity{
            ...
        }

        smtp_reponse = entity{
            ...
        }

        smtp_data = entity{
            ...
        }

        export(smtp_command, smtp_response, smtp_data)
    end)

Specifying terminal tokens
^^^^^^^^^^^^^^^^^^^^^^^^^^
Here, we give the definition of terminal tokens that will be used to specify
smtp messages structure:

.. code-block:: lua

    EMPTY = token('')
    WS = token('[[:blank:]]+')
    CRLF = token('[%r]?[%n]')

    SEP = field('sep', token('[- ]'))
    COMMAND = field('command', token('[[:alpha:]]+'))
    MESSAGE = field('parameter', token('[^%r%n]*'))
    CODE = field('code', token('[0-9]{3}'))
    DATA = field('data', raw_token("[^%r%n]*%r%n"))

    PARAM = record{
        WS,
        MESSAGE
    }

The first three ones are self explanatory. Except for `PARAM`, the rest of tokens are encapsultaed in a `field` element which means that the parsed content will be available for read/write through the provided field name. Finally, `PARAM` is defined using the `record` keyword wich represents a sequence of elements.

Specifying protocol message syntax
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
We distinguish three SMTP messages: commands, responses and data (mail content). Hereafter, we give their specification using the Haka grammar:

SMTP commands
~~~~~~~~~~~~~

Following RFC 2821, smtp command messages are alphabetic chars terminated by un CRLF. Command themselves are followed by white space(s) if paramters are present. These parameters could be required (MAIL, RCPT), optional (HELP) or not permitted at all (DATA, QUIT). These information is stored in the following table which will be used later by the grammar to adapt dissection according to parsed smtp commands:

.. code-block:: lua

    local CMD = {
          ['HELO'] = 'required',
          ['EHLO'] = 'required',
          ['MAIL'] = 'required',
          ['RCPT'] = 'required',
          ['DATA'] = 'none',
         ['RESET'] = 'none',
        ['VERIFY'] = 'required',
        ['EXPAND'] = 'required',
          ['HELP'] = 'optional',
          ['NOOP'] = 'optional',
          ['QUIT'] = 'none'
    }

The syntax of smtp command messages is defined as a `record` starting with a command name (defined previusouly as terminal token) and ending with a trailing CRLF. We use the `branch` keyword to distinguish between the three configuration cases: (i) parameters must follow, (ii) parameters may be present and (iii) no parameter follow. The `branch` entity is endowed with a selection function allowing to select the branch to follow depending on the command name. Note that the gramar has a special element `optional` allowing to handle cases where messages may be present or not. In our case, we detect if paramteres are present by looking one byte further if CRLF is present. This is done thanks to the `lookahead` function (see :doc:`\../../../ref/grammar`)

.. code-block:: lua

    smtp_command = record {
        field('command', COMMAND),
        branch(
            {
                required = PARAM,
                optional = optional(PARAM,
                    function(self, ctx)
                        local la = ctx:lookahead()
                        return not (la == 0xa or la == 0xd)
                    end
                ),
                none = EMPTY
            },
            function (self, ctx)
                return CMD[self.command]
            end
        ),
        CRLF
    }

SMTP responses
~~~~~~~~~~~~~~

A smtp response message is defined as a status code followed by a separator, a comprehensive message and a trailing CRLF:

.. code-block:: lua

    smtp_response = record {
        CODE,
        SEP,
        MESSAGE,
        CRLF
    }

Smtp server may respond by a sequence of response messages which are captured in Haka grammar using an `array` entity. The array size is determined thanks to the `untilcond` option (i.e. hyphen is missing in the last response message):

.. code-block:: lua

    smtp_response = field('responses',
        array(smtp_response):options {
            untilcond = function (elem, ctx)
                return elem and elem.sep == ' '
            end,
        })

.. note:: `untilcond` is an array option that returns true when we reach the end of the array. See :doc:`\../../../ref/grammar` to get the list of available options.

SMTP data
~~~~~~~~~

Finally, data content is defined as following:

.. code-block:: lua

    smtp_data = record {
        DATA
    }

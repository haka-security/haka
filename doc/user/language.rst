.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Haka language
=============
Haka is a Lua-based language designed to provide a quick way to analyze
networking protocols. Lua is a simple scripted language. If you are not familiar
with it, you might find helpful to check the language manual and examples. You
should find many informations on various website, but a good starting point is
the official Lua page at http://www.lua.org/.

Haka API is built upon Lua language. It allows the end-user to define security rules and
to specify protcols and their underlying state machine.

Security rules
--------------
Haka allows users to define security rules in order to filter unwanted packets,
alter their contents, drop them or craft new ones and inject them. A security
rule consists of an event and an evaluation function where the user has full
access (read and write) to all packets fields (headers and data) and a set of
utility functions.

Example:

.. code-block:: lua

    haka.rule {
        on = ipv4.event.receive_packet,
        eval = function (pkt)
            if not pkt:verify_checksum() then
                -- raise an alert
                haka.alert{
                    description = "invalid checksum",
                    sources = { haka.alert.address(pkt.src) },
                }
                -- drop packet
                pkt:drop()
            end
        end
    }


Protocol grammar
----------------
Haka is featured with a grammar allowing to specify both text-based and binary-based
protocols. Its building blocks allow to handle parsing of sequential records as well as of branching records. It supports also many basic types such as bytes, numbers,
regular expressions, etc.

Example: The code below is a snippet of icmp protocol specification

.. code-block:: lua

    icmp_dissector.grammar = haka.grammar:new("icmp")

    icmp_dissector.grammar.packet = haka.grammar.record{
        haka.grammar.field('type',     haka.grammar.number(8)),
        haka.grammar.field('code',     haka.grammar.number(8)),
        haka.grammar.field('checksum', haka.grammar.number(16))
            :validate(function (self)
                self.checksum = 0 
                self.checksum = ipv4.inet_checksum_compute(self._payload)
            end),
        haka.grammar.field('payload',  haka.grammar.bytes())
    }:compile()


Protocol state machine
----------------------
Haka allows to describe protocol state machine which is usefull to manage tcp
transitions for instance. A state machine is defined as a set of states and a
set of transition functions between these states. Haka has predefined states and
transitions to handle common situations such as catching errors and managing
timeouts. 

Example:

.. code-block:: lua

    reset = tcp_connection_dissector.states:state{
        enter = function (context)
            context.flow:trigger('end_connection')
        end,
        timeouts = {
            [60] = function (context)
                return context.states.FINISH
            end
        },
    }

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

HAKA Language
=============
Haka is a Lua-based language designed to provide a quick way to analyze
networking protocols. Its API allows the end-user to define security rules and
to specify protcols and their underlying state machine.

Security rules
--------------
Haka allows users to define security rules in order to filter unwanted packets,
alter their contents, drop them or craft new ones and inject them. A security
rule consists of a hook and an evaluation function where the user has full 
access (read and write) to all packets fields (headers and data) and a set of
utility functions.

Example:

.. code-block:: lua

    haka.rule {
        hook = haka.event('ipv4', 'receive_packet'),
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


Protocol Grammar
----------------
TODO

Protocol State Mahcine
----------------------
TODO
TOD

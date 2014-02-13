.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

UDP
===

.. lua:module:: udp


Dissector
---------

This module register the `udp` dissector.

.. lua:function:: create(ip[, init])

    Create a new UDP packet on top of the given IP packet from
    values passed through the `init` table.

.. lua:class:: udp

    Dissector data for an UDP packet. This dissector is state-less. It will only parse the packet headers.

    .. seealso:: :lua:class:`haka.dissector_data`.

    .. lua:data:: srcport
                  dstport
                  length
                  checksum

        UDP fields as numbers.

    .. lua:data:: payload

        Payload of the packet. Class that contains the udp data payload. The data can be
        accessed using the standard Lua operators `#` to get the length and `[]` to access
        the bytes.

    .. lua:method:: verify_checksum()

        Verify if the checksum is correct.

Example
-------
.. code-block:: lua

    require("protocol/ipv4")
    require("protocol/udp")

    haka.rule {
        hook = haka.event('udp', 'receive_packet'),
        eval = function (pkt)
            haka.log("UDP", "UDP packet from %s:%d to %s:%d", pkt.ip.src,
                pkt.srcport, pkt.ip.dst, pkt.dstport)
        end 
    }



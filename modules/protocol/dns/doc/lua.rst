.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

DNS
===

.. lua:module:: dns


Dissector
---------

This module register the `dns` dissector.

.. lua:class:: dns

    Dissector data for an DNS packet.

    .. seealso:: :lua:class:`haka.dissector_data`.

    .. lua:data:: id
                  qr
                  opcode
                  aa
                  tc
                  rd
                  ra
                  rcode
                  qdcount
                  ancount
                  nscount
                  arcount

        DNS fields as defined by RFC 1035.

    .. lua:data:: question

       DNS Question.

    .. lua:data:: answer
                  authority
                  additional

       DNS answer, authority and additional informations as arrays of :lua:class:`dns.resourcerecord`.

.. lua:class:: dns.resourcerecord

    Resource record as defined by RFC 1035.

    .. lua:data:: name
                  type
	              class
	              ttl
	              length

        DNS resource record fields as defined by RFC 1035.

Example
-------
.. literalinclude:: ../../../../sample/ruleset/dns/pdns.lua
    :language: lua
    :tab-width: 4

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Dns
===

.. haka:module:: dns

Dns dissector module.

**Usage:**

::

    local dns = require('protocol/dns')

Dissector
---------

.. haka:class:: DnsDissector
    :module:
    :objtype: dissector

    :Name: ``'dns'``
    :Extend: :haka:class:`haka.helper.FlowDissector` |nbsp|

    Dissector for the DNS protocol.


Protocol elements
-----------------

.. haka:class:: DnsResult

    DNS message.

    .. haka:attribute:: DnsDissector:id
                        DnsDissector:qr
                        DnsDissector:opcode
                        DnsDissector:aa
                        DnsDissector:tc
                        DnsDissector:rd
                        DnsDissector:ra
                        DnsDissector:rcode
                        DnsDissector:qdcount
                        DnsDissector:ancount
                        DnsDissector:nscount
                        DnsDissector:arcount

        :type: number

        DNS fields as defined by RFC 1035.

    .. haka:attribute:: DnsDissector:question

        :type: :haka:class:`DnsQuestionRecord` array

        DNS Question.

    .. haka:attribute:: DnsDissector:answer
                        DnsDissector:authority
                        DnsDissector:additional

        :type: :haka:class:`DnsResourceRecord` array

        DNS answer, authority and additional informations.

    .. haka:method:: DnsDissector:drop()

        Drop the DNS message.


.. haka:class:: DnsQuestionRecord
    :module:

    Question record as defined by RFC 1035.

    .. haka:attribute:: DnsQuestionRecord:name
                        DnsQuestionRecord:type
                        DnsQuestionRecord:class

        DNS question record fields as defined by RFC 1035.


.. haka:class:: DnsResourceRecord
    :module:

    Resource record as defined by RFC 1035.

    .. haka:attribute:: DnsResourceRecord:name
                        DnsResourceRecord:type
                        DnsResourceRecord:class
                        DnsResourceRecord:ttl
                        DnsResourceRecord:length

        DNS resource record fields as defined by RFC 1035.

    .. note:: The following fields may be present depending on :haka:data:`<DnsResourceRecord>.type`.

    .. haka:attribute:: DnsResourceRecord:ip

        :type: :haka:class:`addr` |nbsp|

        IPv4 object.

    .. haka:attribute:: DnsResourceRecord:name

        :type: string

        Domain name as a string.



Events
------

.. haka:function:: dns.events.query(dns, query)
    :module:
    :objtype: event

    :param dns: DNS dissector.
    :paramtype dns: :haka:class:`DnsDissector`
    :param query: Dns query message.
    :paramtype query: :haka:class:`DnsResult`

    Event triggered whenever a new HTTP request is received.

.. haka:function:: dns.events.response(dns, response, query)
    :module:
    :objtype: event

    :param dns: DNS dissector.
    :paramtype dns: :haka:class:`DnsDissector`
    :param response: Dns response message.
    :paramtype response: :haka:class:`DnsResult`
    :param query: Dns query message associated with the response.
    :paramtype query: :haka:class:`DnsResult`

    Event triggered whenever a new HTTP response is received.


Example
-------
.. literalinclude:: ../../../../sample/ruleset/dns/pdns.lua
    :language: lua
    :tab-width: 4

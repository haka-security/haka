.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

SMTP Protocol dissection
========================

.. highlightlang:: lua

Introduction
------------

It is well known established that hard coding network protocol parsers in
low-level language such as C is error-prone, time-consuming and tedious. Haka features a
new API enabling the specification of network protocols and their underliying
state machine. The resulting specification process leads to a protocol dissector
managing transitions between protocol states and enabling read/write access to
all protocol fields.

Haka grammar covers the specification of text-based (e.g. http) as well as
binary-based protocols (e.g. dns). Thanks to this grammar, we successfully built
several protocol dissectors: ipv4 (with option support), icmp, udp (stateless
and stateful), tcp (stateless and stateful), http (with chunked mode support),
dns. These dissectors are available under ``modules/protocol`` folder in the
source tree

In this tutorial we will cover the specification of smtp protocol using the Haka
grammar. Note that for a sake of convenience, we do not cover all features of
smtp specification but give a guidance on how to write protocol dissectors.

SMTP protocol
-------------
SMTP stands for Simple Mail Transfer Protocol and was designed to deliver mail
reliabily. It is a command/reponses protocol that starts with a session initiation
during which the server sends first a welcoming message together with a status
code indicating if the transaction (220) has succeed or not (554). Then, the
client identifies himself using the commands EHLO ot HELO and awaits for server
response server response to proceed::

    S: 220 foo.com Simple Mail Transfer Service Ready
    C: EHLO bar.com
    S: 250-foo.com greets bar.com
    S: 250-8BITMIME
    S: 250-SIZE
    S: 250-DSN

Mail transaction starts after the above initiation phase. At each step, the
client sends a smtp command and receives one or multiple response messages
from the server. Three steps are required to transfert a mail, namely, through
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

.. _smtp_dissector:

Dissector
---------
In this section, we dive into the dissector module creation.

.. toctree::

    dissector/dissector.rst

.. _smtp_grammar:

Grammar
-------
Haka is featured with grammar allowing to specify both text-based as well as
binary-based protcols. In this section, we give the specification of smtp
protcol using Haka grammar.

.. toctree::
    :titlesonly:

    dissector/grammar.rst

.. _smtp_event:

Events
------
Events are the glue between dissectors and security rules. Dissectors create
events and then trigger them. As a result, all security rules hooking to that
events will be evaluated.


.. toctree::

    dissector/event.rst

.. _smtp_state_machine:

State machine
-------------
Haka allows to specify network protocols and their underlying state machine. A
state machine is defined as a collection of states and transition between these
states.

.. toctree::
    :titlesonly:

    dissector/state_machine.rst


.. _smtp_security:

Security rules
--------------
The purpose of these security rules is to show how use the previously defined
events and how to filter smtp packets based on fields extracted from parsing
results.

.. toctree::

    dissector/security_rule.rst

Full SMTP code
--------------

As a reference, the full specification of the smtp protocol built in this tutorial
can be download here: :download:`smtp.lua<dissector/smtp.lua>`.

Going further
-------------

In this tutorial, we covered the specification of a text-based protocol with Haka.
Interested readers could find in ``modules/protocol`` folder (package sources) the
specification of several protocols such as dns which involve other useful grammar
entities.

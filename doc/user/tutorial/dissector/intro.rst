.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

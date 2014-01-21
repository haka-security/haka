.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Dissector
=========

.. lua:module:: haka.dissector

.. lua:function:: new(d)

Register a new dissector. ``d`` should be a table containg a dissector type (``type`` field)
such as ``EncapsulatedPacketDissector`` and a dissecor name (``name`` field).

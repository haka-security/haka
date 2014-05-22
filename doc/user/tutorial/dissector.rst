.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Creating dissectors - SMTP case study
=====================================

Introduction
------------

* Haka --> Message syntax + state machine
* Haka --> Text-based and binary-based protocols
* Haka dissectors : ip, tcp, http, dns, etc.
* Goal : smtp case study

SMTP Protocol
-------------
* SMTP intro --> mail transfert
* SMTP -> command/response protocol
* Code : snippet of smtp spec : constants (smtp commands tables)
* Goal : basic spec (partial coverage of smtp spec) 

Dissector
---------

Creating the dissector
^^^^^^^^^^^^^^^^^^^^^^

Initializing the dissector
^^^^^^^^^^^^^^^^^^^^^^^^^^

Managing data reception
^^^^^^^^^^^^^^^^^^^^^^^

Adding extras properties and functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Selecting SMTP dissector
^^^^^^^^^^^^^^^^^^^^^^^^

Events
------

Creating events
^^^^^^^^^^^^^^^

Triggering events
^^^^^^^^^^^^^^^^^

Grammar
-------

Grammar building blocks
^^^^^^^^^^^^^^^^^^^^^^^

Specifying terminal tokens
^^^^^^^^^^^^^^^^^^^^^^^^^^

Specifiying protocol message syntax
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

State machine
-------------

Creating the state machine
^^^^^^^^^^^^^^^^^^^^^^^^^^

Adding new states
^^^^^^^^^^^^^^^^^

* Managing session initiation
* Managing command/response comunication
* Managing content mail transfert

Putting all together
--------------------

* Full spec link

Security rules
--------------

* Rule 1: spam filter
* Rule 2: dump mail content

Going further
-------------



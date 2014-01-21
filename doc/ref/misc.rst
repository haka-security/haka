.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Misc
====

Threads
-------

.. lua:module:: haka

.. lua:function:: current_thread()

    Returns the thread current unique index.


Exit
----

.. lua:function:: on_exit(exit_func)

	Runs ``exit_func`` code at haka exit.

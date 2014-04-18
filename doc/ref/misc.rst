.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Misc
====

This section documents various functions that do not fit in any other sections.

Threads
-------

.. haka:module:: haka

.. haka:function:: current_thread() -> id

    :return id: The current thread's index.
    :rtype id: number

    Allows to check what thread is currently running.

.. haka:function:: abort()

    Abort current execution. This function will throw an error that will
    unwind the stack up to the topmost protected call.


Exit
----

.. haka:function:: on_exit(exit_func)

    :param exit_func: Function to run at exit time.
    :paramtype exit_func: function

    Registers a callback that will be called when Haka exits.

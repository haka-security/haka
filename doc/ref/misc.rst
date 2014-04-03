.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Misc
====

This section documents various functions that do not fit in any other sections

Threads
-------

.. lua:module:: haka

.. lua:function:: current_thread()

    Allows to check what thread is running the current code

    :returns: The current thread's index
    :rtype: `integer`


Exit
----

.. lua:function:: on_exit(exit_func)

    Registers a callback that will be called when haka exits

    :param exit_func: the function to run at exit time
    :paramtype exit_func: `function`

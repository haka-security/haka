
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

	Runs ``exit_func`` code at haka exit

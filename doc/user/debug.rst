
Debugging
=========

Interactive rule
----------------

Haka provides a way to write rule interactively using Lua. To do this, you can use a
predefined function ``haka.interactive_rule``.

For instance:

.. code-block:: lua

    haka.rule{
        hooks = { 'ipv4-up' },
        eval = haka.interactive_rule
    }

As a result, every time this rule will be evaluated, a prompt will allow to enter commands. The
current dissector data are available in the variable ``input``. You can use the `TAB` key to get
completion. This can be very useful to discover the available functions and fields.

When you are done, you can let Haka continue its execution by hitting CTRL-D.

.. note::

    As the edition will add a lot of delay, it is best to use the interactive rule on pcap files.
    Otherwise, you can run into problems with tcp for instance.

Debugger
--------

If you need to inspect an existing configuration, you can use the debugger. You need to activate
the debugger first. This can be done by starting haka with the option ``--luadebug``.

This line will start the debugger and break immediately. A prompt will allow you to inspect variables,
up-values, expressions... You can also set breakpoints and execute your code line by line. To get the
list of all available commands, simply type ``help``.


.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Debugging
=========

Interactive rule
----------------

Haka provides a way to filter packet interactively. To do this, you can use a
predefined function ``haka.interactive_rule``.

For instance:

.. code-block:: lua

    haka.rule{
        hook = ipv4.events.receive_packet,
        eval = haka.interactive_rule("interactive")
    }

As a result, every time this rule will be evaluated, a prompt will allow to enter commands. The
current dissector data are available in a table named ``inputs``. Following the above example, typing `inputs[1]` on the prompt will dump ip packet content along with available functions. 

Note that You can use the `TAB` key to get completion. This can be very useful to discover the available functions and fields.

When you are done, you can let Haka continue its execution by hitting CTRL-D.

.. note::

    As the edition will add a lot of delay, it is best to use the interactive rule on pcap files.
    Otherwise, you can run into problems with tcp for instance.

Debugger
--------

If you need to inspect an existing configuration, you can use the debugger. You need to activate
the debugger first. This can be done by starting Haka with the option ``--luadebug``.

This line will start the debugger and break immediately. A prompt will allow you to inspect variables,
up-values, expressions... You can also set breakpoints and execute your code line by line. To get the
list of all available commands, simply type ``help``.


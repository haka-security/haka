.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Hellopacket
===========

As a basic example to get up to speed with Haka this section will help you to
learn how to create basic rules, access data from the packets and display
logging information and security alerts.

Setting up my first Haka script
-------------------------------

Haka configuration is done with script file. Haka scripts are mainly Lua
files. To put a first step into Haka we are going to write a small script aiming
to log every ip packet passing through Haka.

Let's get started by opening your first script with your favorite editor:

.. admonition:: Exercise

    .. code-block:: console

        $ editor hellopacket.lua

Modules
^^^^^^^

Each Haka script must tells Haka which modules it will need. In Haka, dissectors
*are* modules. As our goal is to log every ip packet we will need ``ip``
module.

.. code-block:: lua

    local ipv4 = require('protocol/ipv4')

Lua allow modules to be set in a local variable. Every Haka module use this
feature to avoid contaminating the global state.  Haka provides only one keyword on
the global state: ``haka``. This is the starting point of every functionality
provided by Haka.

Rules
^^^^^

Haka is *event* based. It will trigger event to let you verify some property and
react if needed. You can declare these checks and reactions with Haka *rules*.

If we sum up, Haka rules are made of:

    * an event to attach to
    * a Lua function to check and react

We can declare our first rule by adding the following lines in our ``hellopacket.lua`` script:

.. code-block:: lua

    haka.rule{
        on = ipv4.events.receive_packet,
        eval = function (pkt)
            -- Do some check on pkt and react if necessary
        end
    }

Each dissector will trigger its own events. The event you attach to determines
the arguments passed to the ``eval`` function. You can find a full list of the
available events and their arguments in the documentation.

.. seealso:: `Ipv4 <../../../modules/protocol/ipv4/doc/ipv4.html>`_ where
    you will find the event list of ipv4 dissector.

Logs
^^^^

Finally, Haka provides some log and alert facilities. As you might already have
guess, you can find it under ``haka`` keyword. Now we can finish our first Haka
script by adding the following line in the eval function inside the rule:

.. code-block:: lua

    haka.log("packet from %s to %s", pkt.src, pkt.dst)

Additionally you can throw an alert with ``haka.alert``. For example the
following code will throw an alert of low severity:

.. code-block:: lua

    haka.alert{
        description = "A simple alert",
        severity = "low"
    }

.. note:: The alerts are targeted to network administrators, they contain security
    information that could be reported in some kind of monitoring tool. You can check
    the alert description in the reference guide.

.. admonition:: Exercise

    Write your first haka rule using logging and alert facilities.

Running the script
^^^^^^^^^^^^^^^^^^

Now that your first script is finished you might want to see it in action. You
can simply use ``hakapcap`` to test it on a provided pcap file (:download:`hellopacket.pcap`):

.. code-block:: console

    $ hakapcap hellopacket.lua hellopacket.pcap

Full script
^^^^^^^^^^^

For ease of simplicity you can download the full script here :download:`hellopacket.lua`.

Interactive rule
----------------

.. admonition:: Optional


    Now you should have all the prerequisite to have some fun with Haka. But soon
    you will complain that it can be quite boring to go back and forth between your
    script and the documentation.

    In order to ease the development steps of your script Haka provides a magic eval
    function named ``interactive_rule``. If you use it, Haka will stop and you will
    see a prompt which allow to enter Lua commands.

    Here is an example of an interactive rule:

    .. code-block:: lua

        local ipv4 = require('protocol/ipv4')

        haka.rule{
            on = ipv4.events.receive_packet,
            eval = haka.interactive_rule("my_interactive_rule_on_ip")
        }

    It is strongly recommended to use interactive rules on pcap. If you try to use
    it on real traffic and if you are not fast enough you will encounter lots of
    retransmit packets.

    .. ansi-block::
        :string_escape:

        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mload module 'packet/pcap.ho', Pcap Module
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mload module 'alert/file.ho', File alert
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0msetting packet mode to pass-through

        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mloading rule file 'sample.lua'
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0minitializing thread 0
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mdissector:\x1b[0m \x1b[0mregister new dissector 'raw'
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mpcap:\x1b[0m \x1b[0m     opening file '/usr/share/haka/sample/smtp_dissector/smtp.pcap'
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mdissector:\x1b[0m \x1b[0mregister new dissector 'ipv4'
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m      1 rule(s) on event 'ipv4:receive_packet'
        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m      1 rule(s) registered

        \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m     starting single threaded processing

        \x1b[0m\x1b[32minteractive rule:\x1b[0m
        inputs = table {
          1 : userdata ipv4 {
            checksum : 58353
            dst : userdata addr 192.168.20.1
            flags : userdata ipv4_flags
            frag_offset : 0
            hdr_len : 20
            id : 47214
            len : 60
            name : "ipv4"
            payload : userdata vbuffer
            proto : 6
            raw : userdata packet
            src : userdata addr 192.168.10.10
            tos : 0
            ttl : 63
            version : 4
          }
        }

        Hit ^D to end the interactive session
        my_interactive_rule_on_ip>

    Once you have your prompt you can simply use the ``inputs`` variable to see what
    kind of arguments is passed to your evaluation function.

    .. ansi-block::
        :string_escape:

        my_interactive_rule_on_ip> inputs[1].ttl
          #1    64
        my_interactive_rule_on_ip>

    .. note:: You can use `tab` to auto-complete your commands

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Hellopacket
===========

Introduction
------------
Every language needs a "helloworld".
Haka, being as much a language as a network tool, needs its own helloworld, called
"hellopacket".

This "hellopacket" reads a pcap file and print a couple of tcp/ip fields of each packet in the file.

How-to
------
Launch ``hakapcap`` with a pcap file and a lua script file as arguments.

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/hellopacket
    $ hakapcap hellopacket.pcap hellopacket.lua

Hakapcap will first dump infos about registered dissectors and
rules and then process the pcap file, outputing information on each packet (packet source
and destination, connection establishment, etc.):

.. ansi-block::
    :string_escape:

    \x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mload module 'packet/pcap.ho'
        Pcap Module, Arkoon Network Security
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mloading rule file 'hellopacket.lua'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0minitializing thread 0
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mpcap:\x1b[0m \x1b[0mopenning file 'hellopacket.pcap'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mregistering new dissector: 'ipv4'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mregistering new dissector: 'tcp'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mregistering new dissector: 'tcp_connection'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m1 rule(s) on hook 'ipv4-up'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m1 rule(s) on hook 'tcp_connection-new'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m2 rule(s) registered
    
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mstarting single threaded processing
    
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.1 to 192.168.10.99
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mTCP connection from 192.168.10.1:47161 to 192.168.10.99:3000
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.99 to 192.168.10.1
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.1 to 192.168.10.99
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.1 to 192.168.10.99
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.99 to 192.168.10.1
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.1 to 192.168.10.99
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.99 to 192.168.10.1
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mHello:\x1b[0m \x1b[0mpacket from 192.168.10.1 to 192.168.10.99
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m  \x1b[0munload module 'Pcap Module'
    \x1b[0m


Each new connection and each packet is properly logged. The pcap file is a standard format that
can be opened by various network tools, including wireshark.

Below is the content of the ``hellopacket.lua`` file:

.. literalinclude:: ../../../sample/hellopacket/hellopacket.lua
   :language: lua
   :tab-width: 4


Going further
-------------

All fields can be accessed, read and modified. The fields
are  named similarly to wireshark. For example, you can get the IP `version`, `ttl`
or `proto` simply by using ``pkt.version``, ``pkt.ttl`` or ``pkt.proto``.

.. seealso:: :haka:mod:`ipv4` for a list of all ipv4 accessors.
.. seealso:: :haka:mod:`tcp` for a list of all tcp accessors.


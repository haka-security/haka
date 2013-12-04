
Hellopacket
===========

Introduction
------------
Every language implements its own "helloworld".
Haka is a language, therefore here is its own helloworld called
"hellopacket".

This "hellopacket" reads a pcap file, then print some tcp/ip packet fields.

How-to
------
Launch ``hakapcap`` with a pcap file and a lua script file as arguments.

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/hellopacket
    $ hakapcap hellopacket.pcap hellopacket.lua

As shown below, hakapcap will first dump infos about registered dissectors and
rules and then process the pcap file and ouput networking infos (packet source
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
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0mregistering new dissector: 'tcp-connection'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m1 rule(s) on hook 'ipv4-up'
    \x1b[0m\x1b[1minfo\x1b[0m  \x1b[36mcore:\x1b[0m \x1b[0m1 rule(s) on hook 'tcp-connection-new'
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


We can see each packet and TCP connection saying hello to the world.
You can open the pcap file with wireshark or any other network tool to see the
same informations.

The lua config file is self-documented:

.. literalinclude:: ../../../sample/hellopacket/hellopacket.lua
   :language: lua
   :tab-width: 4


Going further
-------------

All fields from capture can be accessed, read, and/or modified. All the fields
are similar to wireshark syntax. For example, you can get the IP `version`, `ttl`
or `proto` simply by using ``pkt.version``, ``pkt.ttl`` or ``pkt.proto`` on ipv4
rules, respectively.

.. seealso:: Check :lua:mod:`ipv4` to get the full list of ipv4 accessors.


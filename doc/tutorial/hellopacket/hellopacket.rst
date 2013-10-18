
Hellopacket
===========

Introduction
------------
Every language implements its own "helloworld".
Haka is a language, therefore here is its own helloworld called
"hellopacket".

This "hellopacket" reads a pcap file, then print some tcp/ip packet fields.

How to use hellopacket
----------------------
Launch ``hakapcap`` with pcapfile and lua script file as arguments.

.. parsed-literal::

    $ cd |haka_install_path|/share/haka/sample/tutorial/hellopacket
    $ hakapcap hellopacket.pcap hellopacket.lua

Hakacap will process the pcap file accordingly to lua configfile, you will see at
first some infos about the starting of haka, then the packets and stream found in
pcap file:

.. code-block:: bash

    info  core: load module 'packet/pcap.ho'
        Pcap Module, Arkoon Network Security
    info  core: loading rule file 'hellopacket.lua'
    info  core: initializing thread 0
    info  pcap: openning file 'hellopacket.pcap'
    info  core: registering new dissector: 'ipv4'
    info  core: registering new dissector: 'tcp'
    info  core: registering new dissector: 'tcp-connection'
    info  core: 1 rule(s) on hook 'ipv4-up'
    info  core: 1 rule(s) on hook 'tcp-connection-new'
    info  core: 2 rule(s) registered
    
    info  core: starting single threaded processing
    
    info  debug: Hello packet from 192.168.10.1 to 192.168.10.99
    info  debug: Hello TCP connection from 192.168.10.1:47161 to 192.168.10.99:3000
    info  debug: Hello packet from 192.168.10.99 to 192.168.10.1
    info  debug: Hello packet from 192.168.10.1 to 192.168.10.99
    info  debug: Hello packet from 192.168.10.1 to 192.168.10.99
    info  debug: Hello packet from 192.168.10.99 to 192.168.10.1
    info  debug: Hello packet from 192.168.10.1 to 192.168.10.99
    info  debug: Hello packet from 192.168.10.99 to 192.168.10.1
    info  debug: Hello packet from 192.168.10.1 to 192.168.10.99
    info  core:  unload module 'Pcap Module'

We can see each packet and TCP connection saying hello to the world.
You can open the pcap file with wireshark or any other network tool to see the same informations.

The lua config file is self-documented:

.. literalinclude:: ../../../sample/tutorial/hellopacket/hellopacket.lua
   :language: lua
   :tab-width: 4

Haka is very powerful because all of these fields can be used through the lua programming language
without any limit.

Going further
-------------

All fields from capture can be accessed, read, and modified with the lua file.
All the fields are similar to wireshark syntax. For example, you can see the IP version, ttl or proto
simply by using ``pkt.version``, ``pkt.ttl`` or ``pkt.proto``

    .. seealso:: Check :lua:mod:`ipv4` to get more information about


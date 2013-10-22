
Filter
======

Introduction
------------
Haka is a tool which can filter packet and streams, based on any fields or
combination of fields in the packet or the streams..

How to use filter
-----------------
This tutorial is divided in two parts. The fisrt one rely with hakapcap
tool and pcap file, in order to show how to do basic filtering.
The second one uses the nfqueue interface in order to manipulate the
flow as it passes by.

Basic IP and TCP filtering
--------------------------
In order to do some basic IP filtering you can read the self documented lua config file:

.. literal-include:: ../../../sample/tutorial/filter/ipfilter.lua
   :language: lua
   :tab-width: 4

And TCP filtering goes the same 
.. literal-include:: ../../../sample/tutorial/filter/tcpfilter.lua
   :language: lua
   :tab-width: 4

.. code-block:: bash

    hakapcap trace.pcap ipfilter.lua
    hakapcap trace.pcap tcpfilter.lua


Using nfqueue and IP / TCP filtering
------------------------------------
The lua configuration files can be used with nfqueue and haka daemin. Haka will 
hook itself at startup to the raw nfqueue table in order to inspect, modify, 
create and delete packets in real time.

For the rest of this tutorial, we will assume that you've installed
haka package on a router with two NIC (eth1, eth2), like this
setup:

(TODO: make a real png image)
client -- (eth0)haka router(eth1) -- server

This is the configuration of the daemon:

.. literal-include:: ../../../sample/tutorial/filter/daemon.conf
   :language: lua
   :tab-width: 4

which start with:
.. parsed-literal:: 
   # haka -c |haka-install-path|/share/haka/sample/tutorial/filter/daemon.conf \
          -f |haka-install-path|/share/haka/sample/tutorial/filter/ipfilter.conf

The filtering will be done according to the .lua configuration file seen
previously.

.. literal-include:: ../../../sample/tutorial/filter/ipfilter.lua
   :language: lua
   :tab-width: 4

You can adapt the IP/ports/services accordingly and check that packet
are effectively blocked/accepted.

Advanced HTTP filtering
-----------------------
You can filter through all HTTP fields, thanks to http module:

.. literal-include:: ../../../sample/tutorial/filter/httpfilter.lua
   :language: lua
   :tab-width: 4


Packet injection and modifying http data
----------------------------------------
Haka can also alter stream and create packet/data to modify behavior.
We want to be able to filter all obsolete Web browser by their
User-Agent. More, we want to be able to replace content by a message
telling user that its browser is obsolete, and only authorize requests
to updates sites based on their domain name.

.. literal-include:: ../../../sample/tutorial/filter/httpadvancedfilter.lua
   :language: lua
   :tab-width: 4


Going further
-------------


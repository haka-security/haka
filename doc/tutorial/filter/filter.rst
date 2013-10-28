
Filter
======

Introduction
------------
Haka is a tool which can filter packet and streams, based on any fields or
combination of fields in the packet or the streams.

How to use filter
-----------------
This tutorial is divided in two parts. The fisrt one rely on with hakapcap
tool and pcap file, in order to show how to do basic filtering.
The second one uses the nfqueue interface in order to manipulate the
flow as it passes by.

Basic IP and TCP filtering
--------------------------
In order to do some basic IP filtering you can read the self
documented lua config file:

.. literalinclude:: ../../../sample/tutorial/filter/ipfilter.lua
   :language: lua
   :tab-width: 4

And TCP filtering goes the same

.. literalinclude:: ../../../sample/tutorial/filter/tcpfilter.lua
   :language: lua
   :tab-width: 4

A pcapfile is provided in order to see the effects of these two
config files.

.. parsed-literal::

    $ cd |haka_install_path|/share/haka/sample/tutorial/filter/
    $ hakapcap trace.pcap ipfilter.lua
    $ hakapcap trace.pcap tcpfilter.lua


Using nfqueue and IP / TCP filtering
------------------------------------
The lua configuration files can be used with nfqueue and haka daemon.
Haka will hook itself at startup to the raw nfqueue table in order to
inspect, modify, create and delete packets in real time.

For the rest of this tutorial, we will assume that you've installed
haka package on a host with an interface named eth0.

This is the configuration of the daemon:

.. literalinclude:: ../../../sample/tutorial/filter/daemon.conf
   :language: lua
   :tab-width: 4

In order to start haka, you have to be root. The ``--no-daemon`` option
won't send haka daemon on background. Plus, all logs messages are 
printed on output, instead of syslogd.

.. parsed-literal::
   # cd |haka_install_path|/share/haka/sample/tutorial/filter/
   # haka -c daemon.conf --no-daemon

The filtering will be done according to the .lua configuration file seen
previously.

You can adapt the IP accordingly and check that packet
are effectively blocked/accepted. You can also use modify the
``daemon.conf`` file in order to use the ``tcpfilter.lua`` file.

Advanced HTTP filtering
-----------------------
You can filter through all HTTP fields, thanks to http module:

.. literalinclude:: ../../../sample/tutorial/filter/httpfilter.lua
   :language: lua
   :tab-width: 4

Use a daemon configuration file to use with this file:

.. parsed-literal::
   # cd |haka_install_path|/share/haka/sample/tutorial/filter/
   # haka -c httpfilter.conf --no-daemon


Modifying http responses
------------------------
Haka can also alter data sent by webserver. We want to be able to
filter all obsolete Web browser by their User-Agent. More, we want
to force these obsolete browsers to go only to update websites.
Haka will check User-Agent, and if the User-Agent is considered
obsolete, it will change HTTP response to redirect to a safer
site (web site of the browser). The interesting part is that in
no place an IP address is used. So, even if browser uses CDN to
update, it will work.

.. literalinclude:: ../../../sample/tutorial/filter/httpmodif.lua
   :language: lua
   :tab-width: 4

Use a daemon configuration file to use with this file:

.. parsed-literal::
   # cd |haka_install_path|/share/haka/sample/tutorial/filter/
   # haka -c httpmodif.conf --no-daemon

Going further
-------------
This sample could go further. We can filter for any fields, and
modify any part of the response. We can extend the browser list
to match all of major browser version (IE, Chrome, Opera, Safari
and so on).

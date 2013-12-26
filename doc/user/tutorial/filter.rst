.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Filter
======

Introduction
------------
This chapter will document how to use haka to filter packets and streams according to their different fields.

This tutorial is divided into two parts. The first part will use hakapcap and a pcap file to do some basic, offline, filtering. The second part will use nfqueue to alter tcp streams as they pass through haka.

Basic TCP/IP filtering
----------------------

Basic IP filtering
^^^^^^^^^^^^^^^^^^

The direcctory <haka_install_path>/share/haka/sample/filter/ contains all the example files for this section.

A basic filter script on IP fields is provided as ``ipfilter.lua``:

.. literalinclude:: ../../../sample/filter/ipfilter.lua
   :language: lua
   :tab-width: 4

This script can be run with a pcap file provided in the sample directory.

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/filter/
    $ hakapcap ipfilter.pcap ipfilter.lua

To create a pcap file containing all packets that were not dropped, run the following command

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/filter/
    $ hakapcap trace.pcap ipfilter.lua -o output.pcap


The resulting pcap file can be opened with wireshark to check that haka correctly filtered the packets
according to their IP source address.

Interactive rule debugging
^^^^^^^^^^^^^^^^^^^^^^^^^^

Haka allows to interactively debug lua rules. A script containing a lua error is provided as ``tcpfilter.lua``

This script will filter TCP packets, only allowing packets to/from port 80 to pass through.

.. literalinclude:: ../../../sample/filter/tcpfilter.lua
   :language: lua
   :tab-width: 4

To run this example, use the following commands:

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/filter/
    $ hakapcap tcpfilter.pcap tcfilter.lua

When this script is run, haka will output a high number of errors, complaining that the field ``destport`` doesn't exist. We will use haka's debug facilities to find out precisely where the error occurs.

.. seealso:: :doc:`\../debug` contains documentation on haka's debugging facilities.

To start haka in debugging mode, add ``--luadebug`` at the end of the command line:

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/filter/
    $ hakapcap tcpfilter.pcap tcpfilter.lua --luadebug

When a lua error code occurs, the debugger breaks and outputs the error and a backtrace.

.. ansi-block::
    :string_escape:

    \x1b[32mdebug\x1b[1m>  \x1b[0mcontinue
    ...
    \x1b[0m\x1b[32mentering debugger\x1b[0m: unknown field 'destport'
    Backtrace
    \x1b[31m\x1b[1m=>\x1b[0m0    \x1b[36m[C]\x1b[0m: in function '\x1b[35m(null)\x1b[0m'
     #1    \x1b[36m[C]\x1b[0m: in function '\x1b[35m(null)\x1b[0m'
     #2    \x1b[36m[C]\x1b[0m: in function '\x1b[35m__index\x1b[0m'
     #3    \x1b[36mtcpfilter.lua:21\x1b[0m: in function '\x1b[35meval\x1b[0m'
     #4    \x1b[36m/opt/haka/share/haka/core/rule.bc:0\x1b[0m: in the main chunk
     #5    \x1b[36m/opt/haka/share/haka/core/rule.bc:0\x1b[0m: in the main chunk
     #6    \x1b[36m/opt/haka/share/haka/core/rule.bc:0\x1b[0m: in the main chunk
     #7    \x1b[36m[C]\x1b[0m: in function '\x1b[35mxpcall\x1b[0m'
     #8    \x1b[36m/opt/haka/share/haka/core/rule.bc:0\x1b[0m: in the main chunk
    ...

The general syntax of the debugger is close to the syntax of gdb.

Here we are interested in the third frames which is the one in the lua script itself.

To set the debugger to focus on that particular frame, type ``frame 3``. We can now use the ``list`` command to display the faulty source code:

.. ansi-block::
    :string_escape:

    \x1b[32mdebug\x1b[1m>  \x1b[0mlist
    \x1b[33m  16:  \x1b[0mhooks = { 'tcp-up' },
    \x1b[33m  17:  \x1b[0m    eval = function (self, pkt)
    \x1b[33m  18:  \x1b[0m        -- The next line will generate a lua error:
    \x1b[33m  19:  \x1b[0m        -- there is no 'destport' field. replace 'destport' by 'dstport'
    \x1b[31m  20\x1b[1m=> \x1b[0m        if pkt.destport == 80 or pkt.srcport == 80 then
    \x1b[33m  21:  \x1b[0m            haka.log("Filter", "Authorizing trafic on port 80")
    \x1b[33m  22:  \x1b[0m        else
    \x1b[33m  23:  \x1b[0m            haka.log("Filter", "Trafic not authorized on port %d", pkt.dstport)
    \x1b[33m  24:  \x1b[0m            pkt:drop()
    \x1b[33m  25:  \x1b[0m        end

We now see that lua is complaining about an unknonw field ``destport`` on the line testing the destination port of the packet.

Packets, like all structures provided by haka, can be printed easily using the debugger. 

To see the content of the packet, type ``print pkt``:

.. ansi-block::
    :string_escape:

    \x1b[32mdebug\x1b[1m>  \x1b[0mprint pkt
      #1  \x1b[34;1muserdata\x1b[0m tcp {
              ...
            \x1b[34;1mchecksum\x1b[0m : 417
            \x1b[34;1mres\x1b[0m : 0
            \x1b[34;1mnext_dissector\x1b[0m : \x1b[35;1m"tcp-connection"\x1b[0m
            \x1b[34;1msrcport\x1b[0m : 37542
            \x1b[34;1mpayload\x1b[0m : \x1b[35;1muserdata\x1b[0m tcp_payload
            \x1b[34;1mip\x1b[0m : \x1b[36;1muserdata\x1b[0m ipv4 {
              ...
            }
            \x1b[34;1mflags\x1b[0m : \x1b[36;1muserdata\x1b[0m tcp_flags {
              ...
            }
            \x1b[34;1mack_seq\x1b[0m : 0
            \x1b[34;1mseq\x1b[0m : 3827050607
            \x1b[34;1mdstport\x1b[0m : 80
            \x1b[34;1mhdr_len\x1b[0m : 40
        }

You can notice that there is no field called destport. The correct name for the field is ``dstport``. Once this typo is corrected, the script will run properly

Press CTRL-C to quit or type ``help`` to get the list of available commands.

Using rule groups
^^^^^^^^^^^^^^^^^

Haka can handle multiple rules as a group.

Rule groups have three functions

The `init` function is called before any rule from the group is applied

The `continue` function is called between each rule of the group and can decide to stop
processing the group at any point

the `fini` function is called after all rules have been run. It is not called if `continue` has forced a cancelation mid-group.

The following example uses the concept of group to implement a simple filter that only accepts connections on port 80 or port 22

.. literalinclude:: ../../../sample/filter/groupfilter.lua
    :language: lua
    :tab-width: 4

Advanced TCP/IP Filtering
-------------------------

Filtering with NFQueue
^^^^^^^^^^^^^^^^^^^^^^
All the examples so far have used ``hakapcap`` to test some recorded packets.

Haka can also use nfqueue to capture packets from a live interface. The following
examples will illustrate how to do that.

When configured to use nfqueue, haka will hook itself up to the `raw` nfqueue table in order to
inspect, modify, create and delete packets in real time.

The rest of this tutorial assumes that the haka package is installed on a host and that that
host has a network interface named eth0

The configuration file for the daemon is given below:

.. literalinclude:: ../../../sample/filter/daemon.conf
   :language: ini
   :tab-width: 4

In order to be able to capture packets, the `haka` daemon needs to be run as root. The `--no-daemon`` option will prevent `haka` from detaching from the command line and will force `haka` to send its outputs to stdout instead of syslog. 

.. code-block:: console

   # cd <haka_install_path>/share/haka/sample/filter/
   # haka -c daemon.conf --no-daemon

The lua file used here is the one from the first tutorial. This filter will refuse all packets coming from `192.168.10.10`

HTTP filtering
^^^^^^^^^^^^^^
Haka comes with an HTTP parser. Using that module it is easy to filter packets using specific fields from HTTP headers

.. literalinclude:: ../../../sample/filter/httpfilter.lua
   :language: lua
   :tab-width: 4

To test this filter you will need to modify ``dameon.conf`` to tell haka to use ``httpfilter.lua``:

.. code-block:: ini

    [general]
    # Select the haka configuration file to use.
    configuration = "httpfilter.lua"

It is now possible to start the daemon using this new configuration.

.. code-block:: console

   # cd <haka_install_path>/share/haka/sample/filter/
   # haka -c dameon.conf --no-daemon


Modifying HTTP responses
^^^^^^^^^^^^^^^^^^^^^^^^
Haka can also be used to modify packets as they pass through the system. The following example will redirect traffic based on HTTP headers.

More specifically, it will detect outdated firefox versions and will redirect all traffic from these browsers to the firefox update site.

.. literalinclude:: ../../../sample/filter/httpmodif.lua
   :language: lua
   :tab-width: 4

Modify the ``dameon.conf`` in order to load the ``httpmodif.lua``
configuration file:

.. code-block:: ini

    [general]
    # Select the haka configuration file to use
    configuration = "httpmodif.lua"

And start it

.. code-block:: console

   # cd <haka_install_path>/share/haka/sample/filter/
   # haka -c dameon.conf --no-daemon

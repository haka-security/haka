
Filtering with Haka
===================

In this section you will learn how to define a security policy in Haka. To do
so, you will be led to use the IP, TCP and HTTP dissector.

Basic IP filtering
------------------

In the previous section we wrote a simple rule to log every IP packet. This
time, we want to block IP packet originating from a forbidden network.  To do so
we need to be able to check that an IP packet is originating from a given
network, let say ``192.168.10.0/27``.

Haka provide some facilities for this kind of IP manipulation. You can see a
full list of IP tools in ipv4 module documentation. Especially you can see that
it is possible to build a network object:

.. code-block:: lua

        local bad_network = ipv4.network("192.168.10.0/27")

And that you can simply check if an IP belongs to this network:

.. code-block:: lua

        bad_network:contains(ipv4.addr("192.168.10.30"))

Finally, you will notice in the documentation that Haka also provide the usual
``drop`` function on every IP packet.

.. code-block:: lua

        pkt:drop()

.. admonition:: Exercise

    Now you have all what you need to create a rule to block IP packet originating
    from a forbidden network. Create a script named `ipfilter.lua` and run it against
    the provided pcap file :download:`ipfilter.pcap`.

    .. code-block:: console

        $ hakapcap ipfilter.lua ipfilter.pcap

    In order to check that your script correctly block some IP packets you can save
    the output to a new pcap.

    .. code-block:: console

        $ hakapcap ipfilter.lua ipfilter.pcap -o output.pcap

    The resulting pcap file can simply be opened with wireshark.

Full script
^^^^^^^^^^^

You will find a full script for it here :download:`ipfilter.lua`.

TCP filtering
-------------

The aim of this section is to be able to block all TCP traffic except the one
directed to port 22 (ssh) and 80 (http).

Obviously, Haka provide a TCP dissector module named ``protocol/tcp_connection``.
You can load it with the usual ``require`` keyword.

As you can see in the corresponding documentation, ``tcp_connection`` provide an
interesting event: ``tcp_connection.events.new_connection``.

This event pass 2 arguments to the ``eval`` function:

    * flow (TcpConnectionDissector) – TCP flow.

    * tcp (TcpDissector) – TCP packet.

.. admonition:: Exercise

    Write a new rule that check the TCP connection port and block it if port
    doesn't match 22 or 80.

    You can then test your configuration against the pcap :download:`tcpfilter.pcap`.

Full script
^^^^^^^^^^^

You can download the full script at :download:`tcpfilter.lua`.

Filtering with NFQueue
----------------------

All the examples so far have used ``hakapcap`` to test some recorded packets.

Haka can also use nfqueue to capture packets from a live interface. The
following examples will illustrate how to do that.

When configured to use nfqueue, Haka will hook itself up to the `raw` nfqueue
table in order to inspect, modify, create and delete packets in real time.

The rest of this tutorial assumes that the Haka package is installed on a host
which has a network interface named eth0.

The configuration file for the daemon is given below (:download:`haka.conf`):

.. literalinclude:: haka.conf
   :language: ini
   :tab-width: 4

In order to be able to capture packets, the `haka` daemon needs to be run as
root. The ``--no-daemon`` option will prevent `haka` from detaching from the
controlling terminal and will force `haka` to send its outputs to ``stdout``
instead of syslog.

.. code-block:: console

   $ sudo haka -c haka.conf --no-daemon

.. admonition:: Exercise

    The Haka script file used here is the one from the previous exercise. This filter
    will only allow connection to the ports 22 and 80.

    The configuration put Haka on the loopback interface, check using ssh and http
    (wget or iceweasel) that those connections are allowed toward 127.0.0.1.

    You can also change the listening interface to *eth0* to check haka on some
    real servers.

Optional: Interactive rule debugging
------------------------------------

Haka allows to interactively debug Haka script file. A script containing a lua error
is provided as :download:`erroneousrule.lua`.

This script will authorize only packets from/to port 80.

.. literalinclude:: erroneousrule.lua
   :language: lua
   :tab-width: 4

To run this example, use the following commands:

.. code-block:: console

    $ hakapcap erroneousrule.lua tcpfilter.pcap

When running this script, Haka will output a high number of errors, complaining
that the field ``destport`` doesn't exist. We will use Haka's debug facilities
to find out precisely where the error occurs.

.. seealso:: `Debugging <../manual/doc/user/debug.html>`_ contains documentation on Haka's debugging facilities.

To start Haka in debugging mode, add ``--debug-lua`` at the end of the command line:

.. code-block:: console

    $ hakapcap erroneousrule.lua tcpfilter.pcap --debug-lua

When a Lua error code occurs, the debugger breaks and outputs the error and a backtrace.

.. ansi-block::
    :string_escape:

       \x1b[0m\x1b[32mentering debugger\x1b[0m: unknown field 'destport'
        thread: 0
        Backtrace
        \x1b[31m\x1b[1m=>\x1b[0m0     \x1b[36m[C]\x1b[0m: in function '\x1b[35m(null)\x1b[0m'
          #1    \x1b[36m[C]\x1b[0m: in function '\x1b[35m(null)\x1b[0m'
          #2    \x1b[36m[C]\x1b[0m: in function '\x1b[35m__index\x1b[0m'
          #3    \x1b[36merroneousrule.lua:18\x1b[0m: in function '\x1b[35msignal\x1b[0m'
          #4    \x1b[36m/opt/haka/share/haka/core/events.bc:0\x1b[0m: in the main chunk
          #5    \x1b[36m/opt/haka/share/haka/core/events.bc:0\x1b[0m: in the main chunk
          #6    \x1b[36m/opt/haka/share/haka/core/context.bc:0\x1b[0m: in the main chunk
          #7    \x1b[36m[string "tcp"]:14\x1b[0m: in function '\x1b[35mreceive\x1b[0m'
          #8    \x1b[36m/opt/haka/share/haka/core/dissector.bc:0\x1b[0m: in the main chunk
          #9    \x1b[36m[C]\x1b[0m: in function '\x1b[35mxpcall\x1b[0m'
         ...

The general syntax of the debugger is close to the syntax of gdb.

Here we are interested in the third frame which is the one in the Lua script itself.

To set the debugger to focus on that particular frame, type ``frame 3``. We can now use the ``list`` command to display the faulty source code:

.. ansi-block::
    :string_escape:

    \x1b[32mdebug\x1b[1m>  \x1b[0mlist
	\x1b[33m  14:  \x1b[0m    hook = haka.event('tcp', 'receive_packet'),
	\x1b[33m  15:  \x1b[0m    eval = function (pkt)
	\x1b[33m  16:  \x1b[0m        -- The next line will generate a lua error:
	\x1b[33m  17:  \x1b[0m        -- there is no 'destport' field. replace 'destport' by 'dstport'
	\x1b[31m  18\x1b[1m=> \x1b[0m        if pkt.destport == 80 or pkt.srcport == 80 then
	\x1b[33m  19:  \x1b[0m            haka.log("Filter", "Authorizing trafic on port 80")
	\x1b[33m  20:  \x1b[0m        else
	\x1b[33m  21:  \x1b[0m            haka.log("Filter", "Trafic not authorized on port %d", pkt.dstport)
	\x1b[33m  22:  \x1b[0m            pkt:drop()
	\x1b[33m  23:  \x1b[0m        end

We now see that Lua is complaining about an unknown field ``destport`` on the line testing the destination port of the packet.

Packets, like all structures provided by Haka, can be printed easily using the debugger.

To see the content of the packet, type ``print pkt``:

.. ansi-block::
    :string_escape:

    \x1b[32mdebug\x1b[1m>  \x1b[0mprint pkt
	  #1	\x1b[36;1muserdata\x1b[0m tcp {
	    	  \x1b[34;1mack_seq\x1b[0m : 0
	    	  \x1b[34;1mchecksum\x1b[0m : 417
	    	  \x1b[34;1mdstport\x1b[0m : 80
	    	  \x1b[34;1mflags\x1b[0m : \x1b[36;1muserdata\x1b[0m tcp_flags {
	    	    \x1b[34;1mack\x1b[0m : \x1b[35;1mfalse\x1b[0m
	    	    \x1b[34;1mall\x1b[0m : 2
	    	    \x1b[34;1mcwr\x1b[0m : \x1b[35;1mfalse\x1b[0m
	    	    \x1b[34;1mecn\x1b[0m : \x1b[35;1mfalse\x1b[0m
	    	    \x1b[34;1mfin\x1b[0m : \x1b[35;1mfalse\x1b[0m
	    	    \x1b[34;1mpsh\x1b[0m : \x1b[35;1mfalse\x1b[0m
	    	    \x1b[34;1mrst\x1b[0m : \x1b[35;1mfalse\x1b[0m
	    	    \x1b[34;1msyn\x1b[0m : \x1b[35;1mtrue\x1b[0m
	    	    \x1b[34;1murg\x1b[0m : \x1b[35;1mfalse\x1b[0m
	    	  }
	    	  \x1b[34;1mhdr_len\x1b[0m : 40
	    	  \x1b[34;1mip\x1b[0m : \x1b[36;1muserdata\x1b[0m ipv4 {
	    	      ...
	    	  }
	    	  ...
	    	  \x1b[34;1msrcport\x1b[0m : 37542
	    	  \x1b[34;1murgent_pointer\x1b[0m : 0
	    	  \x1b[34;1mwindow_size\x1b[0m : 14600
	    	}

You can notice that there is no field called ``destport``. The correct name for the field is ``dstport``. Once this typo is corrected, the script will run properly

Press CTRL-C to quit or type ``help`` to get the list of available commands.

.. note:: You can use `tab` to auto-complete your commands


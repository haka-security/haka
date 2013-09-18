.. highlightlang:: ini

Haka Suite Tool
===============

Haka-Runtime is a collection of three tools: haka, hakactl and hakapcap

haka
----

.. program:: haka

``haka`` is the main program of the suite. It allows to capture packet using either pcap or nfqueue and to filter/alter them according to the specified lua policy file.

Options
^^^^^^^

``haka`` takes the following options:

.. option:: -h, --help

    Display usage and options information

.. option:: --version

    Display version information

.. option:: -d, --debug

    Display debug output

.. option:: --no-daemon

    Do not run haka as daemon

.. option:: -c, --config

    Read setup configuration from given file

Configuration File
^^^^^^^^^^^^^^^^^^
The configuration file is divided into three main topics: general, packet and log

General Directives
""""""""""""""""""

.. describe:: configuration

    Set the path of the lua policy file

.. describe:: thread

    Set the number of thread to use. By default, all system thread will be used

.. describe:: pass-through

    Activate pass-through mode (yes/no option)

Packet Directives
"""""""""""""""""

.. describe:: module

    Set the packet capture module. Possible values are : ``packet/pcap`` and ``packet/nfqueue``

.. describe:: interfaces

    List of comma-separated interfaces

    Example of possible values : ::

            # Capture loopback traffic
            interfaces = "lo"
            # Capture on interface eth1 and eth2 (nfqueue mode only)
            # interfaces = "eth1, eth2"
            # Capture on all interfaces (nfqueue mode only)
            # interfaces = "any"

.. describe:: dump

    Nfqueue's directive. Save output in pcap files (yes/no option)

.. describe:: dump_input

    Nfqueue's directive. Save received packets in the specified pcap file capture

.. describe:: dump_output
    
    Nfqueue's directive. Save not filtered packets in the specified pcap file capture

.. describe:: dump_drop
    
    Nfqueue's directive. Save filtered packets in the specified pcap file capture

    An example to set packet dumping for nfqueue (only revceived and filtered packets will be saved in pcap files) : ::

            dump = true
            dump_input = "/tmp/input.pcap"
            dump_drop = "/tmp/drop.pcap"


.. describe:: file

    Pcap's directive. Read packets from a pcap file. ``interfaces`` must be commented out

.. describe:: output

    Pcap's directive. Save not filtered packets to the specified pcap output file

    Example of capturing packets from a pcap file and saving not filtered ones in a pcap output file : ::

            #interfaces <-- commented out
            file = "/tmp/input.pcap"
            output = "/tmp/output.pcap"

Log Directives
""""""""""""""

.. describe:: log

    Set the logging module.

Service
^^^^^^^

It is possible to launch ``haka`` as a service. When started, ``haka`` loads the default configuration file located at *TODO*

* Starting haka service

    .. code-block:: bash
    
        sudo service haka start

* Stopping haka service

    .. code-block:: bash
            
        sudo service haka stop


* Restarting haka service

    .. code-block:: bash
            
        sudo service haka restart


* Getting status of haka service

    .. code-block:: bash
            
        sudo service haka status


hakapcap
--------

.. program:: hakapcap

``hakapcap`` is a tool that allows to apply lua policy filters on pcap capture files. It takes as input a pcap file, a lua policy file and a list of options:

    .. code-block:: bash
        
         hakatool [options] <pcapfile> <config>

Options
^^^^^^^

.. option:: -h, --help

    Display usage and options information

.. option:: --version

    Display version information

.. option:: -d, --debug
    
    Display debug output
         
.. option:: --pass-through

    Run in pass-through mode (probe mode).

.. option:: -o <output>

    Save not filtered packets

hakactl
-------

.. program:: hakactl

*TODO*

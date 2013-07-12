
Getting started
===============

.. program:: haka

Haka ships with some setup script and a sample configuration file containing some rules
that you can try.

Scripts
-------

``haka`` takes a Lua script as parameter that is used to setup the environment. This
script will be run and will also receive the extra parameters given to haka.

Some scripts are available in ``share/haka``:

* ``capture-nfq.lua``

  Capture the packet on a list of interfaces using NFQUEUE. It takes two parameters:

  * A list on colon separated interfaces (``any`` is also accepted).
  * A configuration file containing the rules.

  Example:

  .. code-block:: bash

      haka capture-nfq.lua eth1:eth2 config.lua

* ``capture-nfq-dump.lua``

  Same as before, except that the received, output and dropped packets will be saved
  to the files ``input.pcap``, ``output.pcap`` and ``drop.pcap``.

* ``capture-pcap.lua``

  Capture the packets using the libpcap on a network interface. It takes two or three parameters:

  * An interface (Its name, or ``any`` keyword).
  * A configuration file containing the rules.
  * An optional file name to save the processed packets.

  The packets will not really be dropped or modified by the rule when using the pcap capture. This
  could lead to various problems. It is better to use the nfqueue capture.

  .. code-block:: bash

      haka capture-pcap.lua any conf.lua

* ``capture-pcapfile.lua``

  Read the packets from a pcap file using the libpcap. It takes two or three parameters:

  * The input pcap file.
  * A configuration file containing the rules.
  * An optional file name to save the processed packets.

  Example:

  .. code-block:: bash

      haka capture-pcapfile.lua input.pcap config.lua

First Configuration
-------------------
This is a fully functional commented configuration file.

.. literalinclude:: ../../../sample/doc/gettingstarted.lua
    :language: lua

Sample Configuration
--------------------

A sample configuration is installed at ``share/haka/sample/standard``. The main file is named
``config.lua`` which can be used directly with haka.

.. code-block:: bash

    haka share/haka/capture-nfq.lua eth0:eth1 share/haka/sample/standard/config.lua

This configuration contains various rules with different dissectors (*ipv4*, *tcp* and *http*).

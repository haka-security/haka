.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Netmap  `packet/netmap`
===================

Description
-----------

The module uses the netmap kernel module to capture packets from a NIC or host stack of a network interface.

.. note:
    To be able to capture packets on a real interface, the process need to be launched with
    the proper permissions.


Prerequisites
-------------

Compilation:
""""""""""""

You have to download your kernel sources and headers. Netmap needs sources in order to patch and compile new network drivers.
In this documents, kernel sources and headers folders will be respectively nammed <ksrcs> and <khdrs>.

.. code-block:: console
	$ git clone https://code.google.com/p/netmap/ netmap 
	$ cd netmap/LINUX
	$ make KSRC=<khdrs> SRC=<ksrcs>
	$ sudo cp netmap/sys/net/netmap* /usr/include/net/

Test NIC to NIC:
""""""""""""""""

The following example consists in filtering between two interfaces eth0(e1000) and eth1(e1000e). 

.. code-block:: console
	# load kernel modules
	$ sudo modprobe -r e1000 e1000e
	$ sudo insmod netmap/LINUX/netmap_lin.ko
	$ sudo insmod netmap/LINUX/e1000/e1000.ko
	$ sudo modprobe ptp
	$ sudo insmod netmap/LINUX/e1000e/e1000e.ko
	# configure network
	$ sudo ifconfig eth0 promisc up
	$ sudo ifconfig eth1 promisc up
	#set the following configuration for haka
	$ cat ~/netmap.conf
	[general]
	configuration = "/opt/haka/share/haka/sample/empty.lua"
	module = "packet/netmap"
	links = "netmap:eth0=netmap:eth1"
	# start haka
	$ sudo /opt/haka/sbin/haka -c ~/netmap.conf


Test NIC to stack:
""""""""""""""""""

The following example consists in filtering between an interface and the host stack

.. code-block:: console
	# load kernel modules
	$ sudo modprobe -r e1000 e1000e
	$ sudo insmod netmap/LINUX/netmap_lin.ko
	$ sudo insmod netmap/LINUX/e1000/e1000.ko
	# configure network
	$ sudo ifconfig eth0 10.0.0.1 netmask 255.255.255.0
	$ sudo ethtool –offload eth0 rx off tx off
	$ sudo ethtool -K eth0 gso off
	#set the following configuration for haka
	$ cat ~/netmap.conf
	[general]
	configuration = "/opt/haka/share/haka/sample/empty.lua"
	module = "packet/netmap"
	links = "netmap:eth0=netmap:eth0^"
	# start haka
	$ sudo /opt/haka/sbin/haka -c ~/netmap.conf


Parameters
----------

.. describe:: links

    semicolon-separated list of link between netmap ring pairs.

    Example of possible values:

    .. code-block:: ini

        # Interfaces to plug eth0 NIC to eth0 host stack
        links = "netmap:eth0=netmap:eth0^"

        # Interfaces to plug eth0 NIC to eth1 NIC
        links = "netmap:eth0=netmap:eth1"

	# Interfaces to plug eth0 NIC RX to eth0 NIC TX
        links = "netmap:eth0>netmap:eth0"


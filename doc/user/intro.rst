.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

What is HAKA
============

Haka is a suite of tool that allows to capture TCP/IP packets and filter
them based on Lua policy files.

Major features
--------------

* Capture/Drop packet using nfqueue
* Read packet either from a pcap file or from interface using libpcap
* Filter packet from Lua policy files
* Alter packet from Lua policy files
* Inject new packet from Lua policy files
* Filter packet in an interactive fashion way
* Debug security rules
* And more features coming soon ...

Authors / Contributors
----------------------

Project initiators:

* Arkoon Network Security
* OpenWide
* Telecom ParisTech

Contributors:

* Jeremy Rosen
* Kevin Denis
* Mehdi Talbi
* Pierre-Sylvain Desse

License
-------

.. literalinclude:: ../../LICENSE.txt

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

hakapcap
========

.. program:: hakapcap

``hakapcap`` is a tool to quickly apply a lua policy file to a pcap file and see the resulting decisions.
on pcap capture files.

.. code-block:: console

     $ hakatool [options] <pcapfile> <luafile>

Options
-------

.. option:: -h, --help

    Display usage and options information.

.. option:: --version

    Display version information.

.. option:: -d, --debug

    Display debug output.

.. option:: --pass-through

    Run in pass-through mode (probe mode).

.. option:: -o <output>

    Save unfiltered packets.

.. option:: --luadebug

    Start `hakapcap` and automatically attach the Lua debugger.

.. option:: pcapfile

    A pcap file containing the packets to filter

.. option:: luafile

    A lua script containing the filtering rules to apply to the pcap file

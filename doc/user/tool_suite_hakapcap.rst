.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

hakapcap
========

.. program:: hakapcap

``hakapcap`` is a tool that allows to apply lua policy filters
on pcap capture files.
It takes as input an optional list of options, a pcap file, and a 
lua policy file:

.. code-block:: console

     $ hakatool [options] <pcapfile> <config>

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

.. option:: --lua-debug

    Start `hakapcap` and automatically attach the Lua debugger.

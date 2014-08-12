.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

hakapcap
========

.. program:: hakapcap

``hakapcap`` is a tool to quickly apply a Haka policy file to a pcap file and
see the resulting decisions.

.. code-block:: console

     $ hakapcap [options] <hakafile> <pcapfile>

Options
-------

.. option:: -h, --help

    Display usage and options information.

.. option:: --version

    Display version information.

.. option:: -d, --debug

    Display debug output.

.. option:: --no-pass-through

    Disable pass-through mode (probe mode).

.. option:: -o <output>

    Save unfiltered packets.

.. option:: --debug-lua

    Start hakapcap and automatically attach the Haka debugger.

.. option:: hakafile

    A Haka policy file.

.. option:: pcapfile

    A pcap file containing the packets to filter.

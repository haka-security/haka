
hakapcap
========

.. program:: hakapcap

``hakapcap`` is a tool that allows to apply lua policy filters on pcap capture files.
It takes as input a pcap file, a lua policy file and a list of options:

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

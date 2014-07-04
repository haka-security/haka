.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Rule set example
================

Introduction
------------

This tutorial introduces a set of Haka script files located at ``<haka_install_path>/share/haka/sample/ruleset`` and which could be ran using the ``hakapcap`` tool and a capture.pcap file:

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/ruleset
    $ hakapcap config.lua capture.pcap

This rule set can also be used for live analysis of traffic with ``haka`` tool,
but it's preferable to adjust some rules or config first.

config.lua main file
--------------------
This script is just a placeholder to include all other relevants files.

.. literalinclude:: ../../../sample/ruleset/config.lua

The `functions.lua` file is a placeholder to put all Lua utils functions.

Protocol directories
--------------------
A directory has been created for ipv4, tcp, dns and http protocols.
Each directory contains one or more Haka policy files. Each one contains a mandatory `dissector.lua` file which loads the required dissectors. There is some other files to enforce protocols controls and security/rules files.
Every file will be used as long as it's called from the main Haka file. You can read those file as an example of how to make a ruleset made of security, filtering and compliance rules.

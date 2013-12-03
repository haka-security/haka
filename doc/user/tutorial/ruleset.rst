Rule set example
================

Introduction
------------
This Rule set is provides as an example of how it is possible
to include multiples lua policy files.

How-to
------
This tutorial introduces a set of lua script files located at <haka_install_path>/share/haka/sample/ruleset and which could be run using the ``hakapcap`` tool and a capture.pcap file:

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/ruleset
    $ hakapcap capture.pcap config.lua

This Rule set can also be used for live analysis of traffic with ``haka`` tool, but it's preferable to adjust some rules or config
first.

config.lua main file
--------------------
This script is just a placeholder to include all other relevants files.

.. literalinclude:: ../../../sample/ruleset/config.lua

The functions.lua file is a placeholder to put all lua functions used more than once in other scripts.

protocols directories
---------------------
A directory has been created for each protocol analysed, e.g.: ipv4, tcp and http.
Each directory contains one or more lua policy files. Each one contains a mandatory dissector.lua file which handle 
the dissector parts. There is some other files to enforce protocols controls and security/rules files.
Every file will be used as long as it's called from the main lua file
You can read those file as an example of how to make a ruleset with security rules, filtering rules and compliances rules.

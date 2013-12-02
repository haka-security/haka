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


functions.lua file
------------------
This script is a placeholder to put all lua functions used more than
once in other scripts.

.. literalinclude:: ../../../sample/ruleset/functions.lua

ipv4 directory
--------------
This directory holds all relevant files to ipv4 analysis.

At first, we have to load the dissector, file ``ipv4/dissector.lua``.

.. literalinclude:: ../../../sample/ruleset/ipv4/dissector.lua

Then, a compliance check is made, to ensure that checksum is correct, that's the role of file ``ipv4/compliance.lua``:

.. literalinclude:: ../../../sample/ruleset/ipv4/compliance.lua

And finally a security rule, file ``ipv4/security.lua`` to drop packet relative to land attacks:

.. literalinclude:: ../../../sample/ruleset/ipv4/security.lua

tcp directory
-------------
This directory holds all relevant files to tcp analysis.

At first, we have to load the dissector, file ``tcp/dissector.lua``

.. literalinclude:: ../../../sample/ruleset/tcp/dissector.lua

Then a security rule, file ``tcp/security.lua`` to drop packets containing shellcode

.. literalinclude:: ../../../sample/ruleset/tcp/security.lua

And finally a groupe of rules, which can be seen more as a traditional firewall rules, file ``tcp/rules.lua`` authorizing only port 22 (ssh) and 80 (http):

.. literalinclude:: ../../../sample/ruleset/tcp/rules.lua

http directory
--------------
This directory holds all relevant files to http analysis.

At first, we have to load the dissector. file ``http/dissector.lua``

.. literalinclude:: ../../../sample/ruleset/http/dissector.lua

Then, a compliance check is made, to enforce protocols control on http, that's the role of file ``http/compliance.lua``:

.. literalinclude:: ../../../sample/ruleset/http/compliance.lua

And a security policy file to detect some web scanners, that's the role of file ``http/security.lua``:

.. literalinclude:: ../../../sample/ruleset/http/security.lua

And finally a custom policy file, ``http/policy.lua`` in order to add a header and a custom log:


.. literalinclude:: ../../../sample/ruleset/http/policy.lua


Getting started
===============

Running Haka
------------

.. program:: haka

``haka`` is primarily intended to be used as a daemon which could be starded by providing an optional
configaration file (using -c option).

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/doc
    $ sudo haka -c gettingstarted.conf

Sample configuration
--------------------

The previous command will load the configuration file ``gettingstarted.conf`` which as commented hereafter
instructs ``haka`` to capture packets from interface eth0 using nfqueue and to filter them based on the lua
policy script ``gettingstarted.lua``

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.conf
    :language: ini
    :tab-width: 4

First Lua policy file
---------------------

Below, the source code of the lua script file ``gettingstarted.conf`` :

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.lua
    :language: lua
    :tab-width: 4

Sample Lua policy file
----------------------

A sample policy is installed at <haka_install_path>/share/haka/sample/standard. The main file is named
``config.lua`` which can be used directly with haka.

This configuration contains various rules with different dissectors (*ipv4*, *tcp* and *http*).


Getting started
===============

.. program:: haka

``haka`` is primarily intended to be used as a daemon which is started as follows:

    .. code-block:: bash

        sudo service haka start

Sample Configuration
--------------------

The previous command will load the default configuration file ``haka.conf`` which as commented hereafter instructs ``haka`` to capture packet from interface eth0 using nfqueue and to filter them based on the lua policy script ``gettingstartd.lua``

.. literalinclude:: ../sample/doc/gettingstarted.conf
    :language: ini

First Lua Policy File
---------------------
This is a fully functional commented configuration file.

.. literalinclude:: ../sample/doc/gettingstarted.lua
    :language: lua

Sample Lua Policy File
----------------------

A sample policy is installed at ``share/haka/sample/standard``. The main file is named
``config.lua`` which can be used directly with haka.

This configuration contains various rules with different dissectors (*ipv4*, *tcp* and *http*).

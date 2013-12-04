
Getting started
===============

Running Haka
------------

.. program:: haka

``haka`` is primarily intended to be used as a daemon which could be started by providing an optional
configaration file (using -c option).

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/doc/gettingstarted
    $ sudo haka -c gettingstarted.conf

Sample configuration
--------------------

The previous command will load the configuration file ``gettingstarted.conf`` which as commented hereafter
instructs ``haka`` to capture packets from interface eth0 using nfqueue and to filter them based on the lua
policy script ``gettingstarted.lua``

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.conf
    :language: ini
    :tab-width: 4

Lua language
------------

Haka uses the language Lua to define its policy files. It is a simple scripted language. If you are not familiar
with it, you might find helpful to check the language manual and examples. You should find many informations on
various website, but a good starting point is the official Lua page at `<http://www.lua.org/>`_.

First Lua policy file
---------------------

Below, the source code of the lua script file ``gettingstarted.conf`` :

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.lua
    :language: lua
    :tab-width: 4

Going further
-------------

Other documented examples are available to illustrate all features of Haka. Those samples are installed in
``<haka_install_path>/share/haka/sample`` and the corresponding documentation is at :doc:`tutorial`.

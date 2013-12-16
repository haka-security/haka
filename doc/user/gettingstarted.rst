.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Getting started
===============

Running Haka
------------

.. program:: haka

``haka`` is primarily intended to be used as a daemon. This daemon will usually use a configuration file
given on the command line using the `-c` command line option.

The following command will launch haka from the command line using an example configuration file:

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/doc/gettingstarted
    $ sudo haka -c gettingstarted.conf

Sample configuration
--------------------

The content of the file ``gettingstarted.conf`` is detailed below :

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.conf
    :language: ini
    :tab-width: 4

This configuration file instructs ``haka`` to capture packets from interface eth0 using nfqueue and to filter them based on the lua policy script ``gettingstarted.lua``

Lua language
------------

Haka uses the language Lua to define its policy files. It is a simple scripted language. If you are not familiar
with it, you might find helpful to check the language manual and examples. You should find many informations on
various website, but a good starting point is the official Lua page at `<http://www.lua.org/>`_.

First Lua policy file
---------------------

The content of the file ``gettingstarted.lua`` is detailed below :

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.lua
    :language: lua
    :tab-width: 4

Going further
-------------

Other documented examples are available to illustrate all features of Haka. Those samples are installed in
``<haka_install_path>/share/haka/sample`` and the corresponding documentation can be found at :doc:`tutorial`.

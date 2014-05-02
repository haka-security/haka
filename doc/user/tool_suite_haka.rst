.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

haka
====

.. program:: haka

``haka`` is the main program of the collection. It allows to capture packets using either pcap
or nfqueue and to filter/alter them according to the specified Haka policy file.

``haka`` is usually launched as a daemon to monitor packets in the background, but it can 
also be launched from the command line to debug Haka scripts.

Options
-------

``haka`` takes the following options:

.. option:: -h, --help

    Display usage and options information.

.. option:: --version

    Display version information.

.. option:: -d, --debug

    Display debug output.

.. option:: --no-daemon

    Do not run haka as daemon, do not detach from the command line.

.. option:: -c, --config <config>

    Read setup configuration from given file.

.. option:: -r, --rule <rules>

    Override the configuration rule file.

.. option:: --luadebug

    Start haka with debugger capability.

.. option:: --opt <section>:<key>=<value>

    Override a parameter value of the configuration.

Configuration file
------------------

The configuration file is divided into three main sections **general**, **packet**, **alert** and **log**.

General directives
^^^^^^^^^^^^^^^^^^

.. describe:: configuration

    Set the Haka policy file.

.. describe:: thread

    Set the number of threads to use. By default, Haka will use as many threads as cpu-cores.

.. describe:: pass-through=[yes|no]

    Activate pass-through mode. Haka will only monitor traffic and will not allow blocking
    or modification of packets. The overall performence of Haka will be greatly improved.

Packet directives
^^^^^^^^^^^^^^^^^

.. describe:: module

    Set the packet capture module to use.

    .. seealso::

        :ref:`packet_module_section` contains a list of all available modules and
        their options

Alert directives
^^^^^^^^^^^^^^^^

.. describe:: module

    Set the alert module to use.

    .. seealso::

        :ref:`alert_module_section` contains the list of all available modules and
        their options

Log directives
^^^^^^^^^^^^^^

.. describe:: module

    Set the logging module to use.

    .. seealso::

        :ref:`log_module_section` contains the list of all available modules and
        their options

Example
^^^^^^^

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.conf
    :tab-width: 4

Service
-------

On debian, ``haka`` is installed as a system service by the .deb package.
Unless otherwise specified using the `-c` command line option, ``haka`` will
load the default configuration file ``<haka_install_path>/etc/haka/haka.conf``.

* Starting Haka service

    .. code-block:: console

        $ sudo service haka start

* Stopping Haka service

    .. code-block:: console

        $ sudo service haka stop


* Restarting Haka service

    .. code-block:: console

        $ sudo service haka restart


* Getting status of Haka service

    .. code-block:: console

        $ sudo service haka status

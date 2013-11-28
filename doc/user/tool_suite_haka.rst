
haka
====

.. program:: haka

``haka`` is the main program of the suite. It allows to capture packets using either pcap
or nfqueue and to filter/alter them according to the specified lua policy file.

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

    Do not run haka as daemon.

.. option:: -c, --config

    Read setup configuration from given file.

.. option:: --lua-debug

    Start `haka` and automatically attach the Lua debugger.

Configuration file
------------------

The configuration file is divided into three main topics: **general**, **packet**, **alert** and **log**

General directives
^^^^^^^^^^^^^^^^^^

.. describe:: configuration

    Set the path of the Lua policy file.

.. describe:: thread

    Set the number of thread to use. By default, all system thread will be used.

.. describe:: pass-through

    Activate pass-through mode (yes/no option). Pass-through mode allow `haka` to
    be placed as a network probe that cannot interact with the traffic.

Packet directives
^^^^^^^^^^^^^^^^^

.. describe:: module

    Set the packet capture module to use.

    .. seealso::

        Check the :ref:`packet_module_section` to get the list of all available modules and
        their options

Alert directives
^^^^^^^^^^^^^^^^

.. describe:: module

    Set the alert module to use.

    .. seealso::

        Check the :ref:`alert_module_section` to get the list of all available modules and
        their options

Log directives
^^^^^^^^^^^^^^

.. describe:: module

    Set the logging module to use.

    .. seealso::

        Check the :ref:`log_module_section` to get the list of all available modules and
        their options

Example
^^^^^^^

.. literalinclude:: ../../sample/gettingstarted/gettingstarted.conf
    :tab-width: 4

Modules
-------

.. _packet_module_section:

Packet capture modules
^^^^^^^^^^^^^^^^^^^^^^

.. toctree::
    :maxdepth: 1
    :glob:

    ../../modules/packet/*/doc/module*

.. _alert_module_section:

Alert modules
^^^^^^^^^^^^^

.. toctree::
    :maxdepth: 1
    :glob:

    ../../modules/alert/*/doc/module*

.. _log_module_section:

Logging modules
^^^^^^^^^^^^^^^

.. toctree::
    :maxdepth: 1
    :glob:

    ../../modules/log/*/doc/module*

Service
-------

It is possible to launch ``haka`` as a service. When started, ``haka`` loads the
default configuration file ``haka.conf`` located at <haka_install_path>/etc/haka/.

* Starting haka service

    .. code-block:: console

        $ sudo service haka start

* Stopping haka service

    .. code-block:: console

        $ sudo service haka stop


* Restarting haka service

    .. code-block:: console

        $ sudo service haka restart


* Getting status of haka service

    .. code-block:: console

        $ sudo service haka status

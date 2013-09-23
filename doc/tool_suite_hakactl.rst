.. highlightlang:: ini

hakactl
=======

.. program:: hakactl

``hakactl`` allows to control `haka` remotely.

    .. code-block:: bash

         hakactl [options] <command>

Options
-------

.. option:: -h, --help

    Display usage and options information.

.. option:: --version

    Display version information.

Commands
--------

.. option:: status

    Display haka status (running or not).

.. option:: stop

    Stop haka daemon.

.. option:: logs

    Show haka logs in realtime.

.. option:: loglevel <level>

    Set the logging level (fatal, error, warn, info, debug).

    .. seealso:: Check :lua:mod:`haka.log` to get more information about logging levels.

.. option:: debug

    Debug haka rules remotely.

    .. seealso:: Check the :doc:`\debug` topic to get more information about the debugger and the interactive mode.

.. option:: interactive

    Launch the interactive mode remotely.

    .. seealso:: Check the :doc:`\debug` topic to get more information about the debugger and the interactive mode.

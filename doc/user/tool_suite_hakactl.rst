.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

hakactl
=======

.. program:: hakactl

``hakactl`` allows to control `haka` daemon.

.. code-block:: console

     $ hakactl [options] <cmd>

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

.. option:: stats

    Show statistics on packet/bytes captured by haka.

.. option:: logs

    Show haka logs in realtime.

.. option:: loglevel <level>

    Set the logging level (fatal, error, warn, info, debug).

    .. seealso:: Check :lua:mod:`haka.log` to get more information about logging levels.

.. option:: debug

    Debug haka rules remotely.

.. option:: interactive

    Launch the interactive mode remotely.

    .. seealso:: Check the :doc:`\debug` topic to get more information about the debugger and the interactive mode.

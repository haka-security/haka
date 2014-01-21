.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

hakactl
=======

.. program:: hakactl

``hakactl`` allows to control a running `haka` daemon.

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

    .. seealso:: See :lua:mod:`haka.log` for more information about logging levels.

.. option:: debug

    Remotely debug haka rules on a running daemon.

.. option:: interactive

    Put a running daemon in interactive mode

    .. seealso:: See :doc:`\debug` for more information about the debugger and the interactive mode.

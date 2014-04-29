.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Logging
=======

This section introduces the Haka logging API. The logs are used for various type of
information about a running Haka.

.. haka:module:: haka.log

.. haka:function:: fatal(module, fmt[, ...])
                   error(module, fmt[, ...])
                   warning(module, fmt[, ...])
                   info(module, fmt[, ...])
                   debug(module, fmt[, ...])

    :param module: Name of the module issuing the log.
    :paramtype module: string
    :param fmt: Format string.
    :paramtype fmt: string
    :param ...: Format arguments.

    Log a message in corresponding level.

.. haka:function:: haka.log(module, fmt[, ...])

    Alias to :haka:func:`haka.log.info`.

.. haka:function:: message(level, module, fmt[, ...])

    :param level: Level for the log (``'debug'``, ``'info'``, ``'warning'``, ``'error'`` or ``'fatal'``).
    :paramtype level: string
    :param module: Name of the module issuing the log.
    :paramtype module: string
    :param fmt: Format string.
    :paramtype fmt: string
    :param ...: Format arguments.

    Log a message.

.. haka:function:: setlevel(level[, module])

    :param level: Level for the log (``'debug'``, ``'info'``, ``'warning'``, ``'error'`` or ``'fatal'``).
    :paramtype level: string
    :param module: Name of the module to modify.
    :paramtype module: string

    Set the logging level to display. It can be set globally and also manually for
    each module.

Example
-------

::

    haka.log("test", "error in packet %s", pkt)

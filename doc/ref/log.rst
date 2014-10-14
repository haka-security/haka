.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Logging
=======

This section introduces the Haka logging API. The logs are used for various type of
information about a running Haka. The API defines various log sections that allows to
have a fine grained control over information reported.

.. haka:module:: haka

.. haka:function:: haka.log.fatal(fmt[, ...])
                   haka.log.error(fmt[, ...])
                   haka.log.warning(fmt[, ...])
                   haka.log.info(fmt[, ...])
                   haka.log.debug(fmt[, ...])

    :param fmt: Format string.
    :paramtype fmt: string
    :param ...: Format arguments.

    Log a message in corresponding level. This function log to the section named ``'external'``.

.. haka:function:: haka.log(fmt[, ...])

    Alias to :haka:func:`haka.log.info`.


Using different log section
---------------------------

.. haka:function:: log_section(name) -> logging

    If you want more control, you can define a new section with this function. It will
    return a tabe without have the same API as the table :haka:func:`haka.log`.


Example
-------

::

    haka.log("error in packet %s", pkt)

    local log = haka.log_section("mysection")
    log.debug("this is a log")

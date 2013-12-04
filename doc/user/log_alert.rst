.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Logging & Alerting
==================

This section introduces the Haka logging and alerting API

Log
---

.. lua:module:: haka.log

.. lua:function:: fatal(module, fmt, ...)
                  error(module, fmt, ...)
                  warning(module, fmt, ...)
                  info(module, fmt, ...)
                  debug(module, fmt, ...)

    Log a message in various levels.

.. lua:function:: haka.log(module, fmt, ...)

    Alias to :lua:func:`haka.log.info`.

.. lua:function:: setlevel(level)
                  setlevel(module, level)

    Set the logging level to display. It can be set globally and also manually for
    each module.

Alert
-----

.. lua:module:: haka.alert

.. lua:function:: haka.alert{param1=value1, param2=value2, [...]}

    Raise an alert. All parameters are optional.

    :param start_time: time
    :param end_time: time
    :param description: string
    :param severity: one of ``'low'``, ``'medium'`` or ``'high'``
    :param confidence: ``'low'``, ``'medium'``, ``'high'`` or number
    :param completion: ``'failed'`` or ``'successful'``
    :param method: a table
    :param method.description: string
    :param method.ref: table of strings
    :param sources: a table of :lua:func:`haka.alert.address`, :lua:func:`haka.alert.service`...
    :param targets: a table of :lua:func:`haka.alert.address`, :lua:func:`haka.alert.service`...
    :param ref: a table of alert reference
    :returns: Return an alert reference.
    :rtype: Opaque object that can be used in :lua:func:`haka.alert.update`.

.. lua:function:: address(object1, object2, [...])
                  service(object1, object2, [...])

    Create an object to describe a source or a target.

Example:

.. literalinclude:: alert-doc.lua
    :language: lua
    :tab-width: 4

.. lua:function:: update(my_alert, {param1=value1, param2=value2, [...]} )

    Update the alert `my_alert`. The parameters are the same for the alerts.

    :param my_alert: an alert object previously defined
    :param param1,param2,...: Same names/values of alert

Example:

.. literalinclude:: alertupdate-doc.lua
    :language: lua
    :tab-width: 4

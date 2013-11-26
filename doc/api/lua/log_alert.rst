
.. highlightlang:: lua

Logging & Alerting
==================

This section introduces the Haka logging and alerting API

Log :lua:mod:`haka.log`
-----------------------

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

Alert :lua:mod:`haka.alert`
---------------------------

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
    :rtype: Opaque object that can be unsed in :lua:func:`haka.alert.update`.

.. lua:function:: address(object1, object2, [...])
                  service(object1, object2, [...])

    Create an object to describe a source or a target.

Example: ::

    haka.alert{
            start_time = pkt.raw.timestamp,
            description = "packet received",
            severity = 'medium',
            confidence = 'high',
            completion = 'failed',
            method = {
                description = "Packet sent on the network",
                ref = { "cve:2O13-XXX", "http://intranet/vulnid?id=115", "cwe:809" }
            },
            sources = { haka.alert.address(pkt.src, "evil.host.fqdn") },
            targets = { haka.alert.address(pkt.dst), haka.alert.service("tcp/22", "ssh") }
        }

.. lua:function:: update(my_alert, {param1=value1, param2=value2, [...]} )

    Update the alert `my_alert`. The parameters are the same for the alerts.

    :param my_alert: an alert object previously defined
    :param param1,param2,...: Same names/values of alert

Example: ::

    local my_alert = haka.alert{ severity = 'low', sources = { haka.alert.address(pkt.src) } }
    haka.alert.update(my_alert, {completion = 'failed' } )

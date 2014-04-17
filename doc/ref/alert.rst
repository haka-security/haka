.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Alerting
========

This section introduces the Haka alerting API.

.. haka:module:: haka.alert

.. haka:function:: haka.alert{param1=value1, param2=value2[, ...]} -> alert

    :param start_time: Start time of this alert.
    :paramtype start_time: time
    :param end_time: End time of this alert.
    :paramtype end_time: time
    :param description: Description.
    :paramtype description: string
    :param severity: One of ``'low'``, ``'medium'`` or ``'high'``.
    :paramtype severity: string
    :param confidence: ``'low'``, ``'medium'``, ``'high'`` or any user number.
    :paramtype confidence: string or number
    :param completion: State of the attack ``'failed'`` or ``'successful'``.
    :paramtype completion: string
    :param method: Attach method.
    :paramtype method: table
    :param method.description: Description of th method of the attack.
    :paramtype method.description: string
    :param method.ref: List of external reference for this method.
    :paramtype method.ref: table of strings
    :param sources: List of :haka:class:`AlertAddress`, :haka:class:`AlertService`...
    :paramtype sources: table
    :param targets: List of :haka:class:`AlertAddress`, :haka:class:`AlertService`...
    :paramtype targets: table
    :param ref: List of alert references
    :paramtype ref: table
    :return alert: Alert reference.
    :rtype alert: :haka:class:`Alert`

    Raise an alert. All parameters are optional.

.. haka:function:: address(object1, object2, [...]) -> address
                   service(object1, object2, [...]) -> service

    :param object1,object2,...: Any object that can be converted to a string.
    :return address: Address object.
    :rtype address: :haka:class:`AlertAddress`
    :return service: Service object.
    :rtype service: :haka:class:`AlertService`

    Create an object to describe a source or a target.

    .. haka:class:: AlertAddress
        :module:

    .. haka:class:: AlertService
        :module:

Example:

.. literalinclude:: alert-doc.lua
    :language: lua
    :tab-width: 4

.. haka:class:: Alert
    :module:

    Alert object.

    .. haka:method:: Alert:update{param1=value1, param2=value2, [...]}

        :param param1,param2,...: Same names/values of alert (see :haka:func:`haka.alert`)

        Update an existing alert.

    Example:

    .. literalinclude:: alertupdate-doc.lua
        :language: lua
        :tab-width: 4

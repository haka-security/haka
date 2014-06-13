.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Console
=======

The console allows to execute and interact with a running haka daemon. It
is possible to get some statistics or to drop a connection for instance.

Starting the console
--------------------

The console is started using ``hakactl console``. On connection, a prompt
allows to write Lua commands. The completion is also available to guide the
user and list the available functions.

This section provides the list of all available commands.

Returned data
-------------

Most functions return an object containing all requested data. This object
named *List* have many features to enable advanced scripting.

.. haka:class:: List
    :module:

    Collection of elements.

    .. haka:operator:: List[field] -> value

        :param field: Field to retrieve.
        :ptype field: string
        :return list: Field value.
        :return value: Value for the field.

        Get a field value for the given *field*.

    .. haka:method:: List:get(key) -> list

        :param key: Key to retrieve.
        :return list: New list containing only the requested element.
        :rtype list: :haka:class:`List`

        Select only one element from a list.

    .. haka:method:: List:filter(f) -> list

        :param f: Function returning ``true`` for element to keep.
        :ptype f: function
        :return list: New list containing only the element not filtered out.
        :rtype list: :haka:class:`List`

        Filter out and only keep some elements of the list.

    .. haka:method:: List:sort(order, invert=false) -> list

        :param order: String of the field to use for sorting or order function.
        :ptype order: string or function
        :param invert: Invert the result.
        :ptype invert: boolean
        :return list: New list containing only the element not filtered out.
        :rtype list: :haka:class:`List`

        Sort the element of the list.

This object is used by various console commands. Each of them will list the available
fields as well as the extra method available when it is the case.

General utilities
-----------------

.. haka:module:: console

.. haka:function:: threads() -> list
    :module:

    :return list: Threads information.
    :rtype list: :haka:class:`List`

    Get information about the running threads (id, packet statistics, byte statistics...).

.. haka:function:: rules() -> list
    :module:

    :return list: Rules information.
    :rtype list: :haka:class:`List`

    Get information about the loaded rules (name, event...).

.. haka:function:: events() -> list
    :module:

    :return list: Events information.
    :rtype list: :haka:class:`List`

    Get information about all events registered in haka.

.. haka:function:: setloglevel(level[, module])
    :module:

    :param level: Log level (``'debug'``, ``'info'``, ``'warning'``, ``'error'`` or ``'fatal'``).
    :ptype level: string
    :param module: Module name if needed.
    :ptype module: string

    Change the logging level of haka.

.. haka:function:: stop()
    :module:

    Cleanly stop the running haka.

Protocol utilities
------------------

.. include:: ../../modules/protocol/udp/doc/console.rsti
.. include:: ../../modules/protocol/tcp/doc/console.rsti

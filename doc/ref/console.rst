.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Console
=======

This section describe the API to add new console utilities available through
*hakactl*.

The console utilities need to be installed in the folder ``/opt/haka/share/haka/console``.
Those files will automatically be loaded when ``hakactl console`` is started.

List
----

.. haka:module:: list

The :haka:class:`List` is the main object that is used to return data to the console.

.. seealso:: :doc:`Console User Guide <../user/console>`

.. haka:function:: new(name) -> List

    :return List: List class.
    :rtype List: :haka:class:`List`

    Create a new type of list. The returned object can be specialized with fields, formatters...

.. haka:class:: List
    :module:

    The return object describe this new list and can be extended with new methods like
    a basic class.

    This class also need some class values:

    .. haka:data:: List.field

        :type: table of strings

        List of all fields displayed to the user.

    .. haka:data:: List.key

        :type: string

        Unique key for this list. This key is used when a user call *get()*.
        This key must be one known field.

    .. haka:data:: List.field_format

        :type: table of formatter

        Define which formatter to use when printing the value of the fields. This table
        associates name with a corresponding formatter.

        .. seealso:: :ref:`formatter`

    .. haka:data:: List.field_aggregate

        :type: table of aggregator

        Define aggregate utilities to use to add a summary line for all instance of this
        list. It allows for example to add statistics values to compute a global view.
        This table associates name with a corresponding aggregator.

        .. seealso:: :ref:`aggregator`

    .. haka:attribute:: List:_data

        :type: table of table

        Table of table containing each element of this list. It can be used to write custom
        functions.

    .. haka:method:: List:add(data)

        :type: table

        Add one entry into this list.

    .. haka:method:: List:addall(data)

        :type: table of table

        Add all entries in the data into this list.

.. _formatter:

Formatter
^^^^^^^^^

.. haka:module:: list.formatter

Formatters are functions that convert the value given as argument into a string
that will be printed.

.. haka:data:: unit

    Convert a numeric value into human readable string adding units if needed (k, M, G...).

.. haka:function:: optional(default)

    If the value is ``nil``, replace it by a default value.

.. _aggregator:

Aggregator
^^^^^^^^^^

.. haka:module:: list.aggregator

Aggregators are functions that compute a value from a list of values.

.. haka:data:: add

    Sum all values.

.. haka:function:: replace(override)

    Always replace the value by the given *override*.

New function
------------

Adding a new function can be done by adding functions to the table ``haka.console``. This table
is the environment provided within the console.

Remote execution
^^^^^^^^^^^^^^^^

.. haka:module:: hakactl

The console code is executed by the *hakactl* process. It does not have direct access to *haka*.
To be able to interact with it, it is possible to remotely execute some Lua code and get the
result.

.. haka:function:: remote(thread, f) -> ret

    :param thread: Thread number, ``'any'`` or ``'all'``.
    :ptype thread: string or number
    :param f: Function to execute remotely.
    :ptype f: function
    :return ret: Return values.
    :rtype ret: table

    Execute a Lua function on some haka thread's remotely. The thread number can be:

    * The thread id.
    * ``'all'`` if the code should be executed on each thread.
    * ``'any'`` if the code should be executed on one thread but the id is not important.

    The return value is a table containing the result for each called thread. It will have
    one element if executed on 1 thread or more.

    The data send and received are marshaled, some care need to be taken to transmit minimal
    data.

Example
-------

.. literalinclude:: ../../src/hakactl/lua/event.lua
    :language: lua
    :tab-width: 4

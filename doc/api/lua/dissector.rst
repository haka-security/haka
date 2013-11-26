
.. highlightlang:: lua

Dissector
=========

.. lua:currentmodule:: haka

.. lua:class:: dissector_data

    This class is used to communicate the dissector data to rules or other dissectors.

    .. lua:data:: dissector

        Read-only current dissector name.

    .. lua:data:: next_dissector

        Name of the next dissector to call. This value can be read-only or writable depending
        of the dissector.

    .. lua:method:: valid(self)

        Returns `false` if the data are invalid and should not be processed anymore. This could
        happens if a packet is dropped.

    .. lua:method:: drop(self)

        This is a generic function that is called to drop the packet, data or stream.

    .. lua:method:: forge(self)

        Returns the previous dissector data. This function will be called in a loop to enable for
        instance a dissector to create multiple packets. When no more data is available, the function
        should return `nil`.

.. lua:function:: dissector(d)

    Declare a dissector. The table parameter `d` should contains the following
    fields:

    * `name`: The name of the dissector. This name should be unique.
    * `dissect`: A function that take one parameter. This function is the core of
      the dissector. It will be called with the previous :lua:class:`dissector_data`
      and should return a :lua:class:`dissector_data`.

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Dissector
=========

.. lua:currentmodule:: haka

.. lua:class:: dissector_data

    Communicate the dissector data to rules or other dissectors.

    .. lua:data:: dissector

        Read-only current dissector name.

    .. lua:data:: next_dissector

        Name of the next dissector to call. This value can be read-only or writable depending
        of the dissector.

    .. lua:method:: valid(self)

        :returns: `false` if the data are invalid and should not be processed anymore. This could happens if a packet is dropped.

    .. lua:method:: drop(self)

        This is a generic function that is called to drop the packet, data or stream.

    .. lua:method:: forge(self)

        This function will be called in a loop to enable for instance a dissector to create multiple packets.

        :returns: Previous dissector data. When no more data is available, the function should return `nil`.

.. lua:function:: dissector(d)

    Declare a dissector. The table parameter `d` should contains the following
    fields:

    * `name`: The name of the dissector. This name should be unique.
    * `dissect`: A function that take one parameter. This function is the core of
      the dissector. It will be called with the previous :lua:class:`dissector_data`
      and should return a :lua:class:`dissector_data`.
    * `hooks`: A table containing a list of custom hooks. 

Hooks
=====

Haka has up/down built-in hooks associated with protocol modules such as ipv4 (`ipv4-up`, `ipv4-down`) or tcp (`tcp-up`, `tcp-down`). Custom hooks such as those defined for http module (`http-request`, `http-response`) could be used thanks to the following function:

.. lua:function:: rule_hook(hook-name, dissector-name)

    Trigger the evaluation of rules attached to the given `hook-name`.
    
    :returns: `false` if the evaluation should be stopped. This could happens if a packet is dropped.

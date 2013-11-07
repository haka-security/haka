
.. highlightlang:: lua

Lua API :lua:mod:`haka`
=======================

.. lua:module:: haka

.. lua:function:: current_thread()

    Returns the thread current unique index.

Log :lua:mod:`haka.log`
-----------------------

.. lua:module:: haka.log

.. lua:data:: FATAL
              ERROR
              WARNING
              INFO
              DEBUG

    Logging levels.

.. lua:function:: message(level, module, fmt, ...)

    Log a message.

    :param level: One of the known logging level.
    :param module: Module name.
    :param fmt: Message format.

.. lua:function:: fatal(module, fmt, ...)
                  error(module, fmt, ...)
                  warning(module, fmt, ...)
                  info(module, fmt, ...)
                  debug(module, fmt, ...)

    Aliases to log a message in various levels.

.. lua:function:: haka.log(module, fmt, ...)

    Alias to :lua:func:`haka.log.info`.

.. lua:function:: setlevel(level)
                  setlevel(module, level)

    Set the logging level to display. It can be set globally and also manually for
    each module.

Log :lua:mod:`haka.alert`
-------------------------

.. lua:module:: haka.alert

.. lua:function:: address: a list of strings of address, network, ...
                  service: a list of strings of services, proto, ...

    create a table of strings

.. lua:data:: message

    Log a message. All parameters are optional.

    :param start_time: date in raw.timestamp format
    :param end_time: date in raw.timestamp format
    :param description: string
    :param severity: one of 'low', 'medium' or 'high' string
    :param confidence: a string or number
    :param completion: 'failed' if attack has been blocked
    :param method: a table, made of a description (string) and a ref (table of strings)
    :param sources: a table of haka.alert.address and/or haka.alert.services

Example: ::

    haka.alert{
            start_time = pkt.raw.timestamp,
            end_time = pkt.raw.timestamp,
            description = string.format("filtering IP %s", pkt.src),
            severity = 'medium',
            confidence = 7,
            completion = 'failed',
            method = {
                description = "packet sent on the network",
                ref = { "cve/255-45", "http://...", "cwe:dff" }
            },
            sources = { haka.alert.address(pkt.src, "local.org", "network", 33), haka.alert.service(22, "ssh") },
            targets = { haka.alert.address(ipv4.network(pkt.dst, 22)), haka.alert.address(pkt.dst) }
        }

Packet :lua:mod:`haka.packet`
-----------------------------

.. lua:module:: haka.packet

.. lua:class:: packet

    Object representing a packet.  The data can be accessed using the standard Lua
    operators `#` to get the length and `[]` to access the bytes.

    .. lua:method:: drop()

        Drop the packet.

        .. note:: The object will be unusable after calling this function.

    .. lua:method:: accept()

        Accept the packet.

        .. note:: The object will be unusable after calling this function.

Dissector
---------

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
    * `dissect`: A function that take 1 parameter. This function is the core of
      the dissector. It will be called with the previous :lua:class:`dissector_data`
      and should return a :lua:class:`dissector_data`.

Rule
----

.. lua:class:: rule

    .. lua:data:: hooks

        An array of hook names where the rule should be installed.

    .. lua:method:: eval(self, d)

        The function to call to evaluate the rule.
        
        :param d: The dissector data.
        :paramtype d: :lua:class:`dissector_data`

.. lua:function:: rule(r)

    Register a new rule.

    :param r: Rule description.
    :paramtype r: :lua:class:`rule`

Example: ::

    haka.rule {
        hooks = { "ipv4-up" },
        eval = function (self, pkt)
            if pkt.src == ipv4.addr("192.168.1.2") then
                pkt:drop()
            end
        end
    }

Rule group
----------

Rule group allow to customize the rule evaluation.

.. lua:class:: rule_group

    .. lua:data:: name

        Name of the group.

    .. lua:method:: init(self, d)

        This function is called whenever a group start to be evaluated. `d` is the
        dissector data for the current hook (:lua:class:`dissector_data`).

    .. lua:method:: fini(self, d)

        If all the rules of the group have been evaluated, this callback is
        called at the end. `d` is the dissector data for the current hook
        (:lua:class:`dissector_data`).

    .. lua:method:: continue(self, d, ret)

        After each rule evaluation, the function is called to know if the evaluation
        of the other rules should continue. If not, the other rules will be skipped.
        
        :param ret: Value returned by the evaluated rule.
        :param d: Data that where given to the evaluated rule.
        :paramtype d: :lua:class:`dissector_data`

    .. lua:method:: rule(self, r)

        Register a rule for this group.

        .. seealso:: :lua:func:`haka.rule`.

.. lua:function:: rule_group(rg)

    Register a new rule group. `rg` should be a table that will be used to initialize the
    rule group. It can contains `name`, `init`, `fini` and `continue`.

    :returns: The new group.
    :rtype: :lua:class:`rule_group`

Example: ::

    local group = haka.rule_group {
        name = "mygroup",
        init = function (self, d)
            print("entering group")
        end
    }
    
    group:rule {
        hooks = { "ipv4-up" },
        eval = function (self, pkt)
            if pkt.src ~= ipv4.addr("10.0.0.2") then
                pkt:accept()
            end
        end
    }

External modules
----------------

.. toctree::
    :glob:
    :maxdepth: 1

    ../../../modules/*/*/doc/lua*


.. highlightlang:: lua

Lua API :lua:mod:`haka`
=======================

Application :lua:mod:`haka.app`
-------------------------------

.. lua:module:: haka.app

.. lua:function:: current_thread()

    Returns the thread current unique index.

.. lua:function:: exit()

    Exists the application.

.. lua:function:: install(type, module)

    Install a module into the application.

    The type is a string which value can be one of :

    * `packet`
    * `log`

.. lua:function:: load_configuration(file)

    Set the configuration file containing the rules to load.

Module :lua:mod:`haka.module`
-----------------------------

.. lua:module:: haka.module

.. lua:function:: load(name, ...)

    Load the module `name`. The function also take a list of arguments that
    will be passed to the module initialization function.

    An example to setup nfqueue capture on two interfaces: ::

        local mod = haka.module.load("packet-nfqueue", "eth1", "eth2")
        haka.app.install("packet", mod)

.. lua:function:: path()

    Gets the current module search path.

.. lua:function:: setpath(paths)

   Set the module search path list. The paths are separated by a semi-colon. Should be of the form::

       path1/*;path2/*

.. lua:function:: addpath(path)

   Add a path the the search path list.

.. lua:data:: prefix

    Module prefix.

.. lua:data:: suffix

    Module suffix.

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

.. lua:module:: haka

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
            return nil
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
            return nil
        end
    }

External modules
----------------

.. toctree::
    :glob:
    :maxdepth: 1

    ../../../modules/*/doc/lua*

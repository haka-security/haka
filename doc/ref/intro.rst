.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Introduction
============

This section contains the reference guide for user that wants to write Haka
scripts.

The format of this documentation is presented here.

.. note::

    If you want to contribute to the core of Haka, check the section for
    developers: :doc:`../developer/devindex`.

Module
------

.. haka:module:: test
    :noindex:

A module :obj:`test` is used in this example to show how various elements
of a module can be used.

Every functions and variables will be prefixed by the module name, in our case:
``test.``.

Function
--------

Each function is described using the following format:

.. haka:function:: func(param1, param2[, param3]) -> ret1, ret2
    :noindex:
    :module: test

    :param param1: This is how a parameter is described.
    :ptype param1: parameter type if needed
    :param param2: ...
    :return ret1: This is how returned value are described.
    :rtype ret1: return type if needed
    :return ret2: ...

    A function that is available in a module.

    Then the code will follow this declaration::

        local a, b = test.func("string", 42)

Variable
--------

.. haka:data:: data
    :noindex:
    :module: test

    :type: Data type if needed

    A data available on a module.


Object
------

An object in Lua is basically a table containing functions and properties. Every object is documented as follows:

.. haka:class:: Obj
    :noindex:
    :module:

    .. haka:method:: Obj:func(param1) -> ret1
        :noindex:

        :param param1: This is how a parameter is described.
        :ptype param1: parameter type if needed
        :return ret1: This is how returned value are described.
        :rtype ret1: return type if needed

        This function is a method that need to be called on an instance of the
        type *Obj* in this example.

        The ``:`` here is used to call method. See the Lua programming language
        documentation for more detail about it.

        ::

            local obj = test.build(12)
            obj:func()

    .. haka:attribute:: Obj:attr
        :noindex:

        :type: Attribute type if needed

        Attribute of the object.

        ::

            local obj = test.build(12)
            print(obj.attr)

    .. haka:function:: build(param) -> obj
        :noindex:
        :module: test

        This function is available on a module. However, it is described in this
        object because it is related to it. For instance, the function could
        create an instance of :obj:`Obj`.

Event
-----

Haka uses events that allow security rules to be attached to dissectors for
instance. An event is triggered by an emitter and received by a listener. It
is described as follow:

.. haka:function:: test.events.myevent(param1, param2, ...)
    :module:
    :noindex:
    :objtype: event

    :param param1: This is how a parameter is described.
    :ptype param1: parameter type if needed

    This description defines two properties:

    * The name of the event (*myevent*) and how to reference it in order to listen to it or
      to trigger it.
    * The parameters that a listener will receive.


Rule architecture
=================

Dissectors
----------

The configuration will define a list of dissector. Each dissector is described by:

* Name: unique dissector name
* Dissect function: dissector main function

.. image:: dissector.png

.. seealso:: :lua:func:`haka.dissector`.

Rules
-----

Along with the dissector, the configuration can define rules to apply. Those rules are
assigned to some `hooks`. A rule need the following fields:

* A :lua:data:`hooks` member that contains a array of hook string name. It will be used to install the rule on
  them.
* A :lua:func:`rule.eval` function that is called to evaluate the rule.

.. seealso: :lua:func:`rule`.

Rule hooks
----------

The hooks are the points where the user can install rules. When the system will reach a hook
point, all the rules installed on it will be called. The order of the rule execution matches
the declaration order.

A given dissector automatically defines two hooks:

* `dissector_name`-up: For instance, for the dissector `ipv4`, this hook is named `ipv4-up`.
* `dissector_name`-down

.. image:: dissector-hook.png

It is also possible for a dissector to define custom hooks that matches some specific conditions. It
is then responsible for calling the rules when needed.

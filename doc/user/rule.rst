.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

.. lua:currentmodule:: haka

Defining rules
==============

This section introduces how to define security rules.

Single rule
-----------

As detailed herefater, a rule is made of a list of hooks
and an evaluation function:

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

Example:

.. literalinclude:: ../../sample/ruleset/ipv4/security.lua
    :language: lua
    :tab-width: 4

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

Example:

.. literalinclude:: ../../sample/ruleset/tcp/rules.lua
    :language: lua
    :tab-width: 4

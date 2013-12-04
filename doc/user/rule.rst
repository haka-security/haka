.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Rules
=====

.. lua:module:: haka

Single Rule
-----------

.. lua:class:: rule

    .. lua:data:: hook

        Event that will trigger the evaluation function.

    .. lua:method:: eval(...)

        The function to call to evaluate the rule. The number of paramters depends
        on the registered event.

.. lua:function:: rule(r)

    Register a new rule.

    :param r: Rule description.
    :paramtype r: :lua:class:`rule`

Example: ::

   TODO

Rule Group
----------

Rule group allow to customize the rule evaluation.

.. lua:class:: rule_group

    .. lua:method:: init(...)

        This function is called whenever a group start to be evaluated.

    .. lua:method:: fini(...)

        If all the rules of the group have been evaluated, this callback is
        called at the end.

    .. lua:method:: continue(ret, ...)

        After each rule evaluation, the function is called to know if the evaluation
        of the other rules should continue. If not, the other rules will be skipped.

        :param ret: Value returned by the evaluated rule.

    .. lua:method:: rule(eval)

        Register a rule for this group.

        :param eval: Evaluation function.

.. lua:function:: rule_group(rg)

    Register a new rule group. `rg` should be a table that will be used to initialize the
    rule group. It can contains `hook`, `init`, `fini` and `continue`.

    :returns: The new group.
    :rtype: :lua:class:`rule_group`


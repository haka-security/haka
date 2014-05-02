.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Rules
=====

.. haka:module:: haka

Events
------

Haka core will trigger various events emitted by dissectors for instance. This events allow
the user to place rules to verify some property and react if needed.

To get the list of supported events and for each of them their parameters, check the documentation
of the protocol dissectors (:doc:`hakadissector`).

Single Rule
-----------

.. haka:function:: rule{...}

    :param hook: Event to listen to.
    :paramtype hook: Event
    :param eval: Function to call when the event is triggered.
    :paramtype eval: function
    :param options: List of options for the rule.
    :paramtype options: table

    Register a new rule on the given event.

    .. note:: Options are specifics to the events you are hooking to. See :doc:`hakadissector` for more information.

Example:
^^^^^^^^

.. literalinclude:: ../../sample/filter/ipfilter.lua
    :language: lua
    :tab-width: 4


Interactive rule
----------------

.. haka:function:: interactive_rule(name) -> func

    :param name: Rule name displayed in prompt.
    :ptype name: string
    :return func: Rule function.
    :rtype func: function

    Utility function to create an interactive rule. Use this function to build an *eval* function
    of one of your rule.

    When the rule is evaluated, a prompt will enable the user to enter some commands. The parameters
    given to the rule are available through the table named *inputs*.

    **Usage:**

    ::

        haka.rule{
            hook = ipv4.events.receive_packet,
            eval = haka.interactive_rule("interactive")
        }


Rule Group
----------

.. haka:class:: rule_group
    :module:

    Rule group allows customizing the rule evaluation.

    .. haka:function:: rule_group{...} -> group

        :param hook: Event to listen to.
        :paramtype hook: :haka:class:`Event`
        :param init: Function that is called whenever the event is triggered and before any rule evaluation.
        :paramtype init: function
        :param continue: After each rule evaluation, the function is called to know if the evaluation
            of the other rules should continue. If not, the other rules will be skipped.
        :paramtype continue: function
        :param final: If all the rules of the group have been evaluated, this callback is called
            at the end.
        :paramtype final: function
        :param options: List of options for the rule.
        :paramtype options: table
        :return group: New rule group.
        :rtype group: :haka:class:`rule_group`

        Create a new rule group on the given event.

        .. haka:function:: init(...)
            :noindex:
            :module:

            :param ...: Parameter from the event.

        .. haka:function:: continue(ret, ...) -> should_continue
            :noindex:
            :module:

            :param ret: Result from the previous rule evaluation.
            :param ...: Parameter from the event.
            :return should_continue: ``true`` if the next rule in the group should be evaluated.
            :rtype should_continue: boolean

        .. haka:function:: final(...)
            :noindex:
            :module:

    .. haka:method:: rule_group:rule(eval)

        :param eval: Evaluation function.
        :paramtype eval: function

        Register a rule for this group.

Example:
^^^^^^^^

.. literalinclude:: ../../sample/ruleset/tcp/rules.lua
    :language: lua
    :tab-width: 4


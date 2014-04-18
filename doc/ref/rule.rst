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
of the protocol dissectors (:doc:`extmodules`).

.. haka:function:: event(dissector, event) -> event

    :param dissector: Name of the dissector.
    :paramtype dissector: string
    :param event: Name of the event.
    :paramtype event: string
    :return event: :haka:class:`Event`

    Get a dissector event.
    
    .. haka:class:: Event

Single Rule
-----------

.. haka:function:: rule{...}

    :param hook: Event to listen to.
    :paramtype hook: :haka:class:`Event`
    :param eval: Function to call when the event is triggered.
    :paramtype eval: function
    :param options: List of options for the rule.
    :paramtype options: table

    Register a new rule on the given event.

    **Options:**
    
    .. haka:data:: streamed
        :module:

        :type: boolean

        Run the *eval* function in stream mode. In this mode, the function is only called
        once in a separated state where it can block waiting for data.

Example:
^^^^^^^^

.. literalinclude:: ../../sample/filter/ipfilter.lua
    :language: lua
    :tab-width: 4


Interactive rule
----------------

.. haka:function:: interactive_rule(...)

    Utility function to create an interactive rule. Use this function as the *eval* function
    on one of your rule.

    **Usage:**

    ::

        haka.rule{
            hook = haka.event('ipv4', 'receive_packet'),
            eval = haka.interactive_rule
        }


Rule Group
----------

.. haka:class:: rule_group
    :module:

    Rule group allows customizing the rule evaluation.

    .. haka:function:: rule_group{...} -> group

        :param hook: Event to listen to.
        :paramtype hook: :haka:class:`Event`
        :param init: Function that is called whenever a group start to be evaluated.
        :paramtype init: function
        :param continue: After each rule evaluation, the function is called to know if the evaluation
            of the other rules should continue. If not, the other rules will be skipped.
        :paramtype continue: function
        :param final: If all the rules of the group have been evaluated, this callback is called
            at the end.
        :paramtype final: function
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
            
            :param ...: Parameter from the event.
            
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


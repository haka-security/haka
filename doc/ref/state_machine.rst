.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

State Machine
=============

Description
-----------

.. haka:module:: haka

.. haka:function:: state_machine(name, descr) -> state_machine

    :param name: State machine name.
    :ptype name: string
    :param descr: Description function.
    :ptype descr: function
    :return state_machine: Compiled state machine.
    :rtype state_machine: :haka:class:`StateMachine` |nbsp|

    A dissector that need to use a state machine need to describe it first.
    This function allows to do it.

    The *descr* function is executed to define the states and the transitions
    that are going to be part of the state machine. Its environment allows to
    access all state machine primitives listed in this pages. The functions and
    variables available in this scope are shown prefixed with `state_machine`.

Transition
^^^^^^^^^^

A transition is basically a Lua function:

.. haka:function:: transition(context, ...) -> new_state
    :module:
    :noindex:

    :param context: Context of the state machine instance.
    :return new_state: Next state name or nil if we should stay in the current state.
    :rtype new_state: string

    Transition function. Its name can be one of the special transition name or a
    user name to define a user transition.

Haka defines some special transitions:

.. haka:data:: init
    :module:
    :objtype: transition

    Transition activated at machine state initialization.

.. haka:data:: finish
    :module:
    :objtype: transition

    Transition activated when quitting the state machine.

.. haka:data:: enter
    :module:
    :objtype: transition

    Transition activated when entering a new state.

.. haka:data:: leave
    :module:
    :objtype: transition

    Transition activated when leaving a state.

.. haka:data:: error
    :module:
    :objtype: transition

    Transition triggered on error.

.. haka:data:: timeout
    :module:
    :objtype: transition

    Temporal transition. The timeout transitions are described by a table where the key is the timeout value
    in seconds.

    **Usage:**

    ::

        state{
            timeout = {
                [10] = function (self)
                    print("timeout !")
                end
            }
        }

.. haka:function:: default_transitions{transitions...}
    :module:
    :objtype: state_machine

    Sets default transitions for the state machine. The parameter
    should be a table containing a list of transition methods. All those
    transitions will exists on any state of this state machine.

State
^^^^^

A state is represented by an abstract object:

    .. haka:class:: State

The user can define its own state:

.. haka:function:: state{transitions...} -> state
    :module:
    :objtype: state_machine

    :return state: New state.
    :rtype state: :haka:class:`State`

    Defines a new state and its transitions.

Haka automatically defines some special states that can be used as
return value for transitions:

* ``'error'``: raise an error.
* ``'finish'``: final state which will terminate the state machine instance.

It is also needed to define the initial state:

.. haka:function:: initial(state)
    :module:
    :objtype: state_machine

    :param state: Initial state.
    :ptype state: :haka:class:`State`

    Define the initial state where the state machine should start.

Instance
--------

.. haka:class:: StateMachine
    :module:

    This object contains the state machine compiled description.

    .. haka:method:: state_machine:instanciate(context) -> instance

        :param context: User data that are passed to every transitions.
        :return instance: State machine instance.
        :rtype instance: :haka:class:`state_machine_instance`

        Instanciate the state machine. The *context* object will be given as
        the first parameter for every transition called.

.. haka:class:: state_machine_instance
    :module:

    Instance of a state machine.

    .. haka:method:: state_machine_instance:finish()

        Terminate the state machine. This will also call the transition
        **finish** on the current state.

    .. haka:attribute:: state_machine_instance:current
        :readonly:

        :type: string

        Current state name.

    .. haka:method:: state_machine_instance:transition(name, ...)

        :param name: Transition name.
        :ptype name: string

        Call a transition on the current state.

Example
-------

    ::

        local my_state_machine = haka.state_machine("test", function ()

            A = state{
                -- user transition
                update = function (self)
                    print("update")
                    return 'B' -- jump to the state B
                end
            }

            B = state{
                -- special enter transition
                enter = function (self)
                    return 'finish' -- switch the the special state to terminate
                end
            }

            initial(a) -- start on state A
        end)

        local context = {}
        local instance = my_state_machine:instanciate(context)

        instance:transition('update') -- call the transition 'update' on the state 'A'

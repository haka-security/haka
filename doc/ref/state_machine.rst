.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

State Machine
=============

Description
-----------

.. haka:module:: haka

.. haka:class:: state_machine
    :module:

    A dissector that need to use a state machine need to describe it first. This object allows
    to do it.

    A state machine is a collection of states and a collections of transitions.

    Haka automatically defines some special states:

    * *ERROR*: error state.
    * *FINISH*: final state which will terminate the state machine instance.

    A state is represented by an object:

        .. haka:class:: State

    Haka also defines some special transitions:

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

            states:default{
                timeout = {
                    [10] = function (context)
                        print("timeout !")
                    end
                }
            }

    A transition is basically a function:

        .. haka:function:: transition(context, ...) -> new_state
            :module:
            :noindex:

            :param context: Context of the state machine instance.
            :paramtype context: :haka:class:`state_machine_instance`
            :return new_state: Next state or nil if we should stay in the current state.
            :rtype new_state: :haka:class:`State`

    .. haka:function:: state_machine(name) -> state_machine

        :param name: State machine name.
        :paramtype name: string
        :return state_machine: New state machine.
        :rtype state_machine: :haka:class:`state_machine`

        Create a new state machine.

    .. haka:method:: state_machine:default{...}

        Sets default transitions for the state machine. The parameter
        should be a table containing a list of transition methods. All those
        transitions will exists on any state of this state machine.

    .. haka:method::  state_machine:state{...} -> state

        :return state: New state.
        :rtype state: :haka:class:`State`

        Defines a new state and its transitions.

    .. haka:attribute:: state_machine.initial

        Initial state for this machine. This need to be set by the user.

    .. haka:method:: state_machine:instanciate() -> instance

        :return instance: State machine instance.
        :rtype instance: :haka:class:`state_machine_instance`

        Instanciate the state machine.

Instance
--------

.. haka:class:: state_machine_instance

    Instance of a state machine.

    .. haka:method:: state_machine_instance:finish()

        Terminate the state machine.

    .. haka:attribute:: state
        :readonly:

        Current state.

    .. haka:method:: state_machine_instance:<transition>(...)

        Call a transition on the current state. The name *transition* need to be
        replaced by the name of the transition to call.

    **Example:**

    ::

        local my_state_machine = haka.state_machine("test")

        my_state_machine.a = my_state_machine:state{
            update = function (context)
                print("update")
            end
        }

        my_state_machine.initial = my_state_machine.a

        local instance = my_state_machine:instanciate()

        instance:update() -- call the transition 'update' on the state 'a'

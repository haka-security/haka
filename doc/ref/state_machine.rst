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

    * *init*: transition activated at machine state initialization.
    * *finish*: transition activated when quitting the state machine.
    * *enter*: transition activated when entering a new state.
    * *leave*: transition activated when leaving a state.
    * *error*: transition triggered on error.
    * *timeout*: temporal transition.
        Those transitions are described by a table where the key is the timeout value
        in seconds.

        Example::

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

    .. haka:method:: state_machine_instance:transition(...)

    Call a transition on the current state. The name *transition* need to be
    replaced by the name of the transition to call.

    **Example:**

    ::

        local states = haka.state_machine("test")

        states.a = states:state{
            update = function (context)
                print("update")
            end
        }

        states.initial = states.a

        local instance = states:instanciate()

        instance:update() -- call the transition 'update' on the state 'a'

Example
-------

TODO

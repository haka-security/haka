.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

State Machine
=============

Description
-----------

.. haka:module:: haka.state_machine

.. haka:function:: new(name, descr) -> state_machine

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

A state machine can only have one kind of state. The first thing to do is to
choose this type :

.. haka:function:: state_type(state_type)
    :module:
    :objtype: state_machine

    :param state_type: State type.
    :ptype state_type: :haka:class:`class` extending :haka:class:`State`

    Define the type of state that this state machine will work with. It will determine :

        * the events availables.
        * the parameters passed to transitions.
        * the parameters passed to state machine update function (see :haka:class:`state_machine_instance`).

    **Usage:**

    ::

        state_type(BidirectionnalState)

Then it is required to declare all the states :

.. haka:operator:: <name> = state(...)
    :module:
    :objtype: state_machine

    :param name: State name.
    :ptype name: string
    :param ...: Some parameters depending of state machine state type.
    :return entity: a new state.
    :rtype entity: :haka:class:`State`

    Declares a named state that will be avaible in all the state machine.

Finally it is required to define the initial state:

.. haka:function:: initial(state)
    :module:
    :objtype: state_machine

    :param state: Initial state.
    :ptype state: :haka:class:`State`

    Define the initial state where the state machine should start.

State
^^^^^

Haka provides two kind of states type :

.. haka:class:: State

    Simplest state. It won't do anything except defining some default events that
    will be available from every another state types :

    .. haka:data:: init
        :module:
        :objtype: events

        event triggered on state machine initialization.

    .. haka:data:: enter
        :module:
        :objtype: events

        event triggered right after state machine enter the state.

    .. haka:function:: timeout(seconds)
        :module:
        :objtype: events

        :param seconds: Time in seconds to wait before triggering this event
        :ptype seconds: number

        event triggered after a given elapsed time. The timeout are reset each
        time the state machine change its internal state.

    .. haka:data:: fail
        :module:
        :objtype: events

        event triggered right before state machine enter the fail state.

    .. haka:data:: finish
        :module:
        :objtype: events

        event triggered right before state machine enter the finish state.

    .. haka:data:: leave
        :module:
        :objtype: events

        event triggered right before state machine leave the state.


    State declaration in state machine of this type doesn't
    requires any parameters.

    .. haka:function:: state() -> state
        :module:
        :objtype: state_machine

        :return state: A state.
        :rtype state: :haka:class:`State`

    State transitions will be passed state machine context.

    .. haka:function:: action(self)
        :module: <state>

        :param self: state machine context.
        :ptype self: object

    State machine defined with this type will have the following update function.

    .. haka:function:: update(event)
        :module:
        :objtype: state_machine_instance

        :param event: Event to be triggered on the state machine.
        :ptype event: String

.. haka:class:: BidirectionnalState

    Bidirectionnal state is a more advanced state. It can handle bidirectionnal
    connection and will handle data parsing. For this purpose it defines some more events :

    .. haka:data:: up
        :module:
        :objtype: events

        event triggered when up coming data is received.

    .. haka:data:: down
        :module:
        :objtype: events

        event triggered when down coming data is received.

    .. haka:data:: parse_error
        :module:
        :objtype: events

        event triggered on data parsing error. See :doc:`grammar`.

    .. haka:data:: missing_grammar
        :module:
        :objtype: events

        event triggered when no grammar is defined to handle data.

    In order to be able to parse incoming data it is required to pass
    exported grammar entity (see :doc:`grammar`) to state declaration :

    .. haka:function:: state(gup, gdown)
        :module:
        :objtype: state_machine

        :param gup: Up coming data grammar.
        :ptype gup: :haka:class:`CompiledGrammarEntity`
        :param gdown: Down coming data grammar.
        :ptype gdown: :haka:class:`CompiledGrammarEntity`
        :return state: A state.
        :rtype state: :haka:class:`State`

    State transitions attached to ``events.up`` or ``events.down`` events will be passed
    the following parameters :

    .. haka:function:: action(self, res, ...)
        :module: <state>

        :param self: state machine context.
        :ptype self: object
        :param res: Parse result.
        :ptype res: :haka:class:`ParseResult`
        :param ...: Any another parameters as passed to update function.

    State machine defined with this type will have the following update function.

    .. haka:function:: update(payload, direction, ...)
        :module:
        :objtype: state_machine_instance

        :param payload: Payload of the incoming data.
        :ptype payload: :haka:class:`Iter`
        :param direction: Direction of the event.
        :ptype direction: String ``'up'`` or ``'down'``
        :param ...: Any another parameters that will be passed to transitions.

Haka also declares two special states that are available in all state machines :

.. haka:data:: fail
    :module:

    state reached in case of failure. It will raise an error.

.. haka:data:: finish
    :module:

    final state which will terminate the state machine instance.

Naturally it is possible to define specific state types by extending
:haka:class:`State`. It will allow to redefine update function and available
events.

**Usage:**

::

    local MyState = class.class("MyState", haka.state_machine.State)

    function MyState.method:__init()
        class.super(MyState).__init(self, name)
        table.merge(self._transitions, {
            myevent = {},
        });
    end

    function MyState.method:_update(state_machine, myarg)
            state_machine:transition('myevent', myarg)
    end

Transition
^^^^^^^^^^

A transition is composed of the following :

* a state to be defined on
* an event to attach to
* a check function
* an action to perform
* a state to jump to

A transition is defined with :

.. haka:method:: <state>:on{event, check, action, jump}
    :module: state_machine

    :param event: One of the event defined by state machine state type.
    :param check: An optionnal function to decide wether this transition should be taken or
    not.
    :ptype check: function
    :param action: An optionnal function to make some specific actions.
    :ptype action: function
    :param jump: An optionnal state to go to after executing the action.
    :ptype jump: :haka:class:`State`

    Define a new transition. The parameters passed to action and check function
    depends on state machine state type.

    Only event is a required parameter. But a transition must have one of action
    or jump otherwise it is useless.

    Both action and check function are always passed the same parameters.

Haka allow to define default transitions :

.. haka:function:: any:on{event, check, action, jump}
    :module: state_machine

    :param event: One of the event defined by state machine state type.
    :param check: An optionnal function to decide wether this transition should be taken or
    not.
    :ptype check: function
    :param action: An optionnal function to make some specific actions.
    :ptype action: function
    :param jump: An optionnal state to go to after executing the action.
    :ptype jump: :haka:class:`State`

    Sets default transitions for the state machine. The parameter
    should be a table containing a list of transition methods. All those
    transitions will exists on any state of this state machine.

**Usage:**

::

    any:on{
        event = events.fail,
        action = function()
            haka.alert{
                description = "fail on my state machine",
                severity = 'low',
            }
        end,
    }

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

    .. haka:method:: update(...)

        :param ...: Variable parameters depending on state machine type.

        Update the internal state of the state machine.

Example
-------

    ::

        local TestState = class.class('TestState', haka.state_machine.State)

        function TestState.method:__init(name)
            class.super(TestState).__init(self, name)
            table.merge(self._transitions, {
                test = {},
            });
        end

        local my_state_machine = haka.state_machine("test", function ()
            state_type(TestState)

            foo = state()
            bar = state()

            foo:on{
                event = events.test,
                action  = function (self)
                    print("update")
                end,
                jump = bar -- jump to the state bar
            }

            bar:on{
                event = events.enter,
                action  = function (self)
                    print("finish")
                end
            }

            initial(foo) -- start on state foo
        end)

        local context = {}
        local instance = my_state_machine:instanciate(context)

        instance:update('test') -- trigger the event test

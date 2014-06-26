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

    The *descr* function is executed to define the states and the actions that
    are going to be part of the state machine. Its environment allows to access
    all state machine primitives listed in this pages. The functions and
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
        * the parameters passed to actions.
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
    will be available from every another state types.

    .. haka:data:: State._events

        This table contains a list of the events added by this type of state.
        If you create a class inheriting from :haka:class:`State` you can set
        this table to add some events.

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

    State actions will be passed state machine context.

    .. haka:function:: execute(self)
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

    State actions attached to ``events.up`` or ``events.down`` events will be passed
    the following parameters :

    .. haka:function:: execute(self, res, ...)
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
        :param ...: Any another parameters that will be passed to actions.

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

    MyState._events = { 'myevent' }

    function MyState.method:_update(state_machine, myarg)
            state_machine:trigger('myevent', myarg)
    end

Actions
^^^^^^^

An action is composed of the following :

* a state to be defined on
* an event to attach to
* a check function
* an action to perform
* a state to jump to

A action is defined with :

.. haka:method:: <state>:on{event, when, execute, jump}
    :module: state_machine

    :param event: One of the event defined by state machine state type.
    :param when: An optional function to decide whether this action should be taken or not.
    :ptype when: function
    :param execute: An optional function to make some specific actions.
    :ptype execute: function
    :param jump: An optional state to go to after executing the action.
    :ptype jump: :haka:class:`State`

    Define a new action. The parameters passed to action and when function
    depends on state machine state type.

    Only event is a required parameter. But an action must have one of action
    or jump otherwise it is useless.

    Both action and when function are always passed the same parameters.

Haka allow to define default actions :

.. haka:function:: any:on{event, when, execute, jump}
    :module: state_machine

    :param event: One of the event defined by state machine state type.
    :param when: An optional function to decide whether this action should be taken or not.
    :ptype when: function
    :param execute: An optional function to make some specific actions.
    :ptype execute: function
    :param jump: An optional state to go to after executing the action.
    :ptype jump: :haka:class:`State`

    Sets default actions for the state machine. The parameter should be a
    table containing the exact same argument as a classic action. All those
    actions will exists on any state of this state machine.

**Usage:**

::

    any:on{
        event = events.fail,
        execute = function()
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

        :param context: User data that are passed to every actions.
        :return instance: State machine instance.
        :rtype instance: :haka:class:`state_machine_instance`

        Instanciate the state machine. The *context* object will be given as
        the first parameter for every actions called.

.. haka:class:: state_machine_instance
    :module:

    Instance of a state machine.

    .. haka:method:: state_machine_instance:finish()

        Terminate the state machine. This will also call the action
        **finish** on the current state.

    .. haka:attribute:: state_machine_instance:current
        :readonly:

        :type: string

        Current state name.

    .. haka:method:: state_machine_instance:trigger(name, ...)

        :param name: Transition name.
        :ptype name: string

        Trigger an event on the current state.

    .. haka:method:: update(...)

        :param ...: Variable parameters depending on state machine type.

        Update the internal state of the state machine.

Example
-------

    ::

        local TestState = class.class('TestState', haka.state_machine.State)

        function TestState.method:__init(name)
            class.super(TestState).__init(self, name)
            table.merge(self._actions, {
                test = {},
            });
        end

        local my_state_machine = haka.state_machine("test", function ()
            state_type(TestState)

            foo = state()
            bar = state()

            foo:on{
                event = events.test,
                execute  = function (self)
                    print("update")
                end,
                jump = bar -- jump to the state bar
            }

            bar:on{
                event = events.enter,
                execute  = function (self)
                    print("finish")
                end
            }

            initial(foo) -- start on state foo
        end)

        local context = {}
        local instance = my_state_machine:instanciate(context)

        instance:update('test') -- trigger the event test

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

State Machine
=============

.. lua:module:: haka

.. lua:class:: state_machine

    .. lua:function:: new(name)
 
        Create a new state machine.

        :param name: State machine name
        :paramtype name: string

    .. lua:method:: default(trans)

        Sets default transitions for the state machine. ``trans``
        should be a table containing a list of transition methods.
        Haka defines a list of built-in transitions:

        * init: transition activated at machine state initialisation.
        * finish: transition activated when quitting the state machine.
        * enter: transition activated when entering a new state.
        * leave: transition activated when leaving a state.
        * error: transition triggered on error.
        * timeout: temporal transition.

    .. lua:method:: state(trans)

		Defines a new state and its stransitions.

    .. lua:method:: instanciate()

        Instanciates the state machine.

Instancieted State Machine
--------------------------

TODO

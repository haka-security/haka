.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Context
=======

.. warning:: This section contains advanced feature of Haka.

.. haka:module:: context

.. haka:class:: Context
    :module:

    Haka define a global context and local scopes. Both allow user to register
    events and to trigger them.

    .. haka:method:: signal(emitter, event, ...)

        :param emitter: The emitter of the event.
        :paramtype emitter: Object
        :param event: The event to trigger.
        :paramtype event: Event
        :param ...: Optionnal parameters of the event.

        Trigger an event on the global context and optionnaly on the current
        scope.

    .. haka:method:: newscope()

        Create a new scope.

    .. haka:method:: exec(scope, func)

        :param scope: A local scope.
        :paramtype scope: Scope
        :param func: The function to execute.
        :paramtype func: function

        Execute ``func`` under the given scope.

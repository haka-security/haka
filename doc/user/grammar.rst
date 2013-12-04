.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Grammar
=======

.. lua:module:: haka.grammar

.. lua:function:: record(entities)

    Create record based on list of sub entities.

.. lua:function:: array(entities)

    Create an array of sub entities

.. lua:function:: branch(cases, select)

    :param cases: list of cases
    :param select: selector function

    Create a conditional branching entities.

.. lua:function:: union(entities)

    Create C-union like based entities.

.. lua:function:: field(name, field)

    :param name: field name
    :param field: entity

    Create a field entity.

.. lua:function:: number(n)

    :param n: number of bits
	:paramtype n: integer

    Create an int value using ``n`` bits.

.. lua:function:: bytes()

    Create a byte-based entity.

.. lua:function:: re(regex)

	Create a regular expression entity.


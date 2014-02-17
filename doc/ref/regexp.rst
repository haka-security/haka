.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Regular Expression
==================

Types
-----

.. lua:module:: regexp

.. lua:class:: re

    .. lua:function:: match(pattern, string)

        Compile and match a regular expression against a given string.

        :param pattern: Regular expression pattern
        :paramtype pattern: string
        :param string: String against wich regular expression is matched
        :paramtype string: string

        :return: true if pattern matched, false otherwise

        :raises Error: if pattern compilation fails
        :raises Error: if internal regular expression engine fails

    .. lua:function:: match(pattern, vbuffer)

        Compile and match a regular expression against a given vbuffer.

        :param pattern: Regular expression pattern
        :paramtype pattern: string
        :param vbuffer: vbuffer against wich regular expression is matched
        :paramtype vbuffer: vbuffer

        :return: true if pattern matched, false otherwise

        :raises Error: if pattern compilation fails
        :raises Error: if internal regular expression engine fails

    .. lua:function:: compile(pattern)

        Compile a regular expression.

        :param pattern: Regular expression pattern
        :paramtype pattern: string

        :return: a regexp object

        :raises Error: if pattern compilation fails
        :raises Error: if internal regular expression engine fails

.. lua:class:: regexp

    .. lua:function:: match(string)

        Match the compiled regular expression against a given string.

        :param string: String against wich regular expression is matched
        :paramtype string: string

        :return: true if pattern matched, false otherwise

        :raises Error: if internal regular expression engine fails

    .. lua:function:: match(vbuffer)

        Match the compiled regular expression against a given vbuffer.

        :param vbuffer: vbuffer against wich regular expression is matched
        :paramtype vbuffer: vbuffer

        :return: true if pattern matched, false otherwise

        :raises Error: if internal regular expression engine fails

    .. lua:function:: get_ctx()

        Create a regexp context that can be eventually used for matching the
        regular expression against chunck of data

        :return: a regexp_ctx object

        :raises Error: if internal regular expression engine fails

.. lua:class:: regexp_ctx

    .. lua:function:: feed(string)

        Match the compiled regular expression across multiple string.

        :param string: String against wich regular expression is matched
        :paramtype string: string

        :return: true if pattern matched, false otherwise

        :raises Error: if internal regular expression engine fails

    .. lua:function:: feed(vbuffer)

        Match the compiled regular expression across multiple vbuffer.

        :param vbuffer: vbuffer against wich regular expression is matched
        :paramtype vbuffer: vbuffer

        :return: true if pattern matched, false otherwise

        :raises Error: if internal regular expression engine fails

Example
-------

.. literalinclude:: regexp.lua
    :language: lua
    :tab-width: 4

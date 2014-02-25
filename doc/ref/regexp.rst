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
        :param string: String against which regular expression is matched
        :paramtype string: string

        :rtype: bool
        :rtype: :lua:class:`regexp.regexp_result`
        :return: (true, :lua:class:`regexp.regexp_result`) if pattern matched, (false, nil) otherwise

        :raises Error: if pattern compilation fails
        :raises Error: if internal regular expression engine fails

    .. lua:function:: match(pattern, vbuffer)

        Compile and match a regular expression against a given vbuffer.

        :param pattern: Regular expression pattern
        :paramtype pattern: string
        :param vbuffer: vbuffer against which regular expression is matched
        :paramtype vbuffer: vbuffer

        :rtype: bool
        :rtype: :lua:class:`regexp.regexp_vbresult`
        :return: (true, :lua:class:`regexp.regexp_vbresult`) if pattern matched, (false, nil) otherwise

        :raises Error: if pattern compilation fails
        :raises Error: if internal regular expression engine fails

    .. lua:function:: compile(pattern)

        Compile a regular expression.

        :param pattern: Regular expression pattern
        :paramtype pattern: string

        :rtype: :lua:class:`regexp.regexp`
        :return: a regexp object

        :raises Error: if pattern compilation fails
        :raises Error: if internal regular expression engine fails

.. lua:class:: regexp

    .. lua:function:: match(string)

        Match the compiled regular expression against a given string.

        :param string: String against which regular expression is matched
        :paramtype string: string

        :rtype: bool
        :rtype: :lua:class:`regexp.regexp_result`
        :return: (true, :lua:class:`regexp.regexp_result`) if pattern matched, (false, nil) otherwise

        :raises Error: if internal regular expression engine fails

    .. lua:function:: match(vbuffer)

        Match the compiled regular expression against a given vbuffer.

        :param vbuffer: vbuffer against which regular expression is matched
        :paramtype vbuffer: vbuffer

        :rtype: bool
        :rtype: :lua:class:`regexp.regexp_vbresult`
        :return: (true, :lua:class:`regexp.regexp_vbresult`) if pattern matched, (false, nil) otherwise

        :raises Error: if internal regular expression engine fails

    .. lua:function:: get_sink()

        Create a regexp context that can be eventually used for matching the
        regular expression against chunck of data

        :rtype: :lua:class:`regexp.regexp_sink`
        :return: a regexp_sink object

        :raises Error: if internal regular expression engine fails

.. lua:class:: regexp_sink

    .. lua:function:: feed(string, eof)

        Match the compiled regular expression across multiple string.

        :param string: String against which regular expression is matched
        :paramtype string: string
        :param eof: is this string the last one ?
        :paramtype eof: bool

        :rtype: bool
        :rtype: :lua:class:`regexp.regexp_result`
        :return: (true, :lua:class:`regexp.regexp_result`) if pattern matched, (false, nil) otherwise

        :raises Error: if internal regular expression engine fails

    .. lua:function:: feed(vbuffer, eof)

        Match the compiled regular expression across multiple vbuffer.

        :param vbuffer: vbuffer against which regular expression is matched
        :paramtype vbuffer: vbuffer
        :param eof: is this vbuffer the last one ?
        :paramtype eof: bool

        :rtype: bool
        :rtype: :lua:class:`regexp.regexp_result`
        :return: (true, :lua:class:`regexp.regexp_result`) if pattern matched, (false, nil) otherwise
        :warning: This function **does not return** a :lua:class:`regexp.regexp_vbresult` since we cannot guarantee that the vbuffer that have been fed into this sink are continuous

        :raises Error: if internal regular expression engine fails

.. lua:class:: regexp_result

    A result for a string based matching.

    .. lua:data:: offset (int)
    .. lua:data:: size (int)

.. lua:class:: regexp_vbresult

    A result for a vbuffer based matching.

    .. lua:data:: offset (int)
    .. lua:data:: size (int)

Example
-------

.. literalinclude:: regexp.lua
    :language: lua
    :tab-width: 4

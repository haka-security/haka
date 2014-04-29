.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Regular Expression
==================

.. haka:module:: haka


Regular expression format
-------------------------

The regular expression format depends on the regular expression module that is loaded. Any ``%`` found in
the string will be automatically converted to a ``/``.

Examples for the pcre module::

    '[%r]?%n'
    '[[:blank:]]+'
    '[0-9]+%.[0-9]+'
    '[^()<>@,;:%\\"/%[%]?={}[:blank:]]+'


Module
------

.. haka:class:: regexp_module
    :module:

    Regexp module implementation.

    .. haka:method:: regexp_module:match(pattern, string, options) -> match, begin, end

        :param pattern: Regular expression pattern.
        :paramtype pattern: string
        :param string: String against which regular expression is matched.
        :paramtype string: string
        :param options: Regular expression compilation options.
        :paramtype options: number
        :return match: Result of the matching.
        :rtype match: string
        :return begin: Index in the string where the match begins.
        :rtype begin: number
        :return end: Index in the string where the match ends.
        :rtype end: number

        Match a regular expression against a given string.

    .. haka:method:: regexp_module:match(pattern, buffer, options) -> match
        :noindex:

        :param pattern: Regular expression pattern.
        :paramtype pattern: string
        :param buffer: Buffer against which regular expression is matched.
        :paramtype buffer: :haka:class:`vbuffer`
        :param options: Regular expression compilation options.
        :paramtype options: number
        :return match: Matching sub-buffer or nil if no match
        :rtype match: :haka:class:`vbuffer_sub`

        Match a regular expression against a given buffer.

    .. haka:method:: regexp_module:match(pattern, buffer_iterator, options, createsub) -> match
        :noindex:

        :param pattern: Regular expression pattern.
        :paramtype pattern: string
        :param buffer_iterator: Buffer iterator against which the regular expression is matched.
        :paramtype buffer_iterator: :haka:class:`vbuffer_iterator`
        :param options: Regular expression compilation options.
        :paramtype options: number
        :param createsub: True if the function should build a sub-buffer.
        :paramtype createsub: boolean
        :return match: Matching sub-buffer if `createsub` is true or ``true`` or nil if no match
        :rtype match: :haka:class:`vbuffer_sub`

        Match a regular expression against a given buffer iterator.

        Usage::

            local match = pcre:match("%s+", iter, 0, true)
            print(match:asstring())

    .. haka:method:: regexp_module:compile(pattern, options) -> re

        :param pattern: Regular expression pattern.
        :paramtype pattern: string
        :param options: Regular expression compilation options.
        :paramtype options: number
        :return re: A compiled regexp object.
        :rtype re: :haka:class:`regexp`

        Compile a regular expression.

    .. haka:attribute:: regexp_module.CASE_INSENSITIVE

        Compilation option setting regular expression case insensitive.

    .. haka:attribute:: regexp_module.CASE_EXTENDED

        Compilation option allowing to ignore white space chars


Compiled regular expression
---------------------------

.. haka:class:: regexp
    :module:

    Compiled regular expression object.

    .. haka:method:: regexp:match(string) -> match, begin, end

        :param string: String against which regular expression is matched.
        :paramtype string: string
        :return match: Result of the matching.
        :rtype match: string
        :return begin: Index in the string where the match begins.
        :rtype begin: number
        :return end: Index in the string where the match ends.
        :rtype end: number

        Match the compiled regular expression against a given string.

    .. haka:method:: regexp:match(buffer) -> match
        :noindex:

        :param buffer: Buffer against which the regular expression is matched.
        :paramtype buffer: :haka:class:`vbuffer`
        :return match: Matching sub-buffer or nil if no match
        :rtype match: :haka:class:`vbuffer_sub`

        Match the compiled regular expression against a given buffer.

    .. haka:method:: regexp:match(buffer_iterator) -> match
        :noindex:

        :param buffer_iterator: Buffer iterator against which the regular expression is matched.
        :paramtype buffer_iterator: :haka:class:`vbuffer_iterator`
        :return match: Matching sub-buffer or nil if no match
        :rtype match: :haka:class:`vbuffer_sub`

        Match the compiled regular expression against a given buffer iterator.

    .. haka:method:: regexp:get_sink() -> sink

        :return sink: A created regexp_sink.
        :rtype sink: :haka:class:`regexp_sink`

        Create a regular expression context that can be eventually used for matching the
        regular expression against chunks of data.


Regular expression sink
-----------------------

.. haka:class:: regexp_sink
    :module:

    Sink that can be used to match a regular expression on data pieces by pieces.

    .. haka:method:: regexp_sink:feed(string, eof[, result]) -> match

        :param string: String against which the regular expression is matched.
        :paramtype string: string
        :param eof: True if this string is the last one.
        :paramtype eof: boolean
        :param result: Data used to store the indices of the match.
        :paramtype result: :haka:class:`regexp_result`
        :return match: Result of the matching.
        :rtype match: boolean

        Match the compiled regular expression across multiple strings.

    .. haka:method:: regexp_sink:feed(buffer, eof) -> match, begin, end

        :param buffer: Buffer against which the regular expression is matched.
        :paramtype buffer: :haka:class:`vbuffer_sub`
        :param eof: True if this string is the last one.
        :paramtype eof: boolean
        :return match: Result of the matching.
        :rtype match: boolean
        :return begin: Position of the beginning of the match or nil.
        :rtype begin: :haka:class:`vbuffer_iterator`
        :return end: Position of the end of the match or nil.
        :rtype end: :haka:class:`vbuffer_iterator`

        Match the compiled regular expression across multiple buffer. The `begin` and `end` result allow
        to track the position of the match.

    .. haka:method:: regexp_sink:ispartial() -> partial

        :return match: State of this sink.
        :rtype match: boolean

        Get the the sink is in state. If something has been match but more data are needed
        to be sure that it is a valid match then this function will returns ``true``.

.. haka:class:: regexp_result

    A result for a string based matching.

    .. haka:attribute:: regexp_result.offset

        :type: number

    .. haka:attribute:: regexp_result.size

        :type: number

Example
-------

.. literalinclude:: regexp.lua
    :language: lua
    :tab-width: 4

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Grammar
=======

.. haka:module:: haka.grammar

Haka allows the user to describe a full protocol grammar. This section describes all constructs available
to build rules for the grammar.

.. haka:function:: new(name, descr) -> grammar

    :param name: Name of the grammar.
    :paramtype name: string
    :param descr: Function that describe the grammar rules.
    :ptype descr: function
    :return grammar: Created grammar object.
    :rtype grammar: :haka:class:`Grammar`

    Create a new grammar. The name is mainly used to report detailed
    information to the user.

    The `descr` parameter is a function that describe the grammar rules. Its
    environment allows to access all grammar primitives listed in this pages.
    The functions and variables available in this scope are shown prefixed
    with `grammar`.

    The grammar description must export public elements in order to be able to
    use them and to parse some data. To do this, the :haka:func:`export` function
    allow to notify which elements are going to be used for parsing. You should only
    export root elements that are going to be used as entry point for parsing.

.. haka:function:: export(...)
    :objtype: grammar
    :module:

    :param ...: List of elements to be exported.
    :ptype ...: :haka:class:`GrammarEntity`

    Export some grammar elements to be able to use them for parsing.

.. haka:class:: Grammar
    :module:

    Object holding grammar rules.

    .. haka:operator:: Grammar[name] -> entity

        :param name: Rule name.
        :ptype name: string
        :return entity: Compiled grammar representation.
        :rtype entity: :haka:class:`CompiledGrammarEntity`

        Get a rule for parsing. This rule must have been exported using the
        function :haka:func:`export`.


Elements
--------

A grammar in Haka is made of various elements that are described in this section. Each one, have different
properties.

.. haka:class:: GrammarEntity
    :module:

    Object representing any grammar element. This object have different functions that cnn be used in the
    grammar specification

    .. haka:method:: GrammarEntity:options{...} -> entity

        :return entity: Modified grammar entity.
        :rtype entity: :haka:class:`GrammarEntity`

        Generic option specification function. Check the documentation of the grammar element to get the list
        of option supported.

        The options are passed as ``key=value`` in the table used as the parameter of the function.

    .. haka:method:: GrammarEntity:extra{...} -> entity

        :return entity: Modified grammar entity.
        :rtype entity: :haka:class:`GrammarEntity`

        Method only available on record which can be used to add extra element to it. The table should only
        contain functions.

        Each named element in the array will be added as a extra field in the result.
        The other element will be executed when the record will be done with its parsing.

    .. haka:method:: GrammarEntity:validate(validator) -> entity

        :param validator: Validator function.
        :paramtype validator: function
        :return entity: Modified grammar entity.
        :rtype entity: :haka:class:`GrammarEntity`

        Add a validation function for the element. This function is called when a field is mark invalid by
        setting it to ``nil``.

        .. haka:function:: validator(result)
            :noindex:
            :module:

            :param result: Current parsing result.
            :param context: Full parsing context.

    .. haka:method:: GrammarEntity:convert(converter) -> entity

        :param converter: Value converter.
        :paramtype converter: :haka:mod:`haka.grammar.converter`
        :return entity: Modified grammar entity.
        :rtype entity: :haka:class:`GrammarEntity`

        Set a conversion operation to apply to the element data.


Final elements
^^^^^^^^^^^^^^

.. haka:function:: number(bits) -> entity
    :objtype: grammar
    :module:

    :param bits: Size of the number in bits.
    :paramtype bits: number
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    **Supported options:**

    .. haka:data:: endianness
        :module:
        :idxctx: number
        :objtype: option
        :idxtype: number grammar option

        Endianness of the raw data: ``little`` or ``big``. By default, the data will be treated
        as big endian.

    **Usage:**

    ::

        number(8)

    Parse a binary number.

.. haka:function:: token(pattern) -> entity
    :objtype: grammar
    :module:

    :param pattern: Regular expression pattern for the token.
    :paramtype pattern: string
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Match a regular expression on the data.

    .. note:: The regular expression will surrounded by non-capturing group : ``"^(?:"...")"``.

    **Usage:**

    ::

        token('%s+')

.. haka:data:: flag
    :objtype: grammar
    :module:

    :type: :haka:class:`GrammarEntity`

    Parse a flag of 1 bit and returns it as a ``boolean``.

.. haka:function:: bytes() -> entity
    :objtype: grammar
    :module:

    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Parse a block of data.

    **Supported options:**

    .. haka:data:: count
        :module:
        :idxctx: bytes
        :objtype: option
        :idxtype: bytes grammar option

        :type: number

        Number of bytes.

    .. haka:function:: count(result, context) -> count
        :module:
        :idxctx: bytes
        :objtype: option
        :idxtype: bytes grammar option

        :param result: Current parse result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`
        :return count: Number of bytes.
        :rtype count: number

    .. haka:function:: chunked(result, sub, islast, context)
        :module:
        :idxctx: bytes
        :objtype: option
        :idxtype: bytes grammar option

        :param result: Current parsing result.
        :param sub: Current data block.
        :param islast: True if this data block is the last one.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`

        This option allows to get each data as soon as they are received in a callback function.

.. haka:function:: padding{align=align_bit} -> entity
                   padding{size=size_bit} -> entity
    :objtype: grammar
    :module:

    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Parse some padding. The padding can be given by size or by alignment.

.. haka:function:: field(name, entity) -> entity
    :objtype: grammar
    :module:

    :param name: Name of the field in the result.
    :paramtype name: string
    :param entity: Entity to named.
    :paramtype entity: grammar entity
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Create a named entity. This is used to give access to an entity of the grammar. It
    will then be possible to access to data in the result in a security rule for instance.

    **Usage:**

    ::

        field("WS", token('%s+'))

.. haka:function:: verify(verif, msg) -> entity
    :objtype: grammar
    :module:

    :param verif: Verification function.
    :paramtype verif: function
    :param msg: Error message to report.
    :paramtype msg: string
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Verify some property during the parsing. If ``func`` returns ``false``, then an error is
    reported with ``msg``.

    .. haka:function:: verif(result, context) -> is_valid
        :noindex:
        :module:

        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`
        :return is_valid: False if the verification fails.
        :rtype is_valid: boolean

.. haka:function:: execute(exec) -> entity
    :objtype: grammar
    :module:

    :param exec: Generic function.
    :paramtype exec: function
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Execute a generic function during the parsing. This allows to deeply customize the parsing using
    regular Lua functions.

    .. haka:function:: exec(result, context)
        :noindex:
        :module:

        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`

.. haka:function:: fail(msg) -> entity
    :objtype: grammar
    :module:

    :param msg: Error messgae.
    :paramtype msg: string
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Always fail the parsing when reaching this element.

Compounds
^^^^^^^^^

.. haka:function:: record(entities) -> entity
    :objtype: grammar
    :module:

    :param entities: List of entities for the record.
    :paramtype entities: table of grammar entities
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Create a record for a list of sub entities. Each entity is expected to appear
    one by one in order.

    When working on a stream, the data behind the elements is kept which allow
    transparent access and modification.

    **Usage:**

    ::

        record{
            field('type', number(8)),
            bytes()
        }

.. haka:function:: sequence(entities) -> entity
    :objtype: grammar
    :module:

    :param entities: List of entities for the sequence.
    :paramtype entities: table of grammar entities
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Create a sequence for a list of sub entities. Each entity is expected to appear
    one by one in order.

    This element is similar to the :haka:func:`record` but the data in a stream will
    immediatly be sent on the network.

    **Usage:**

    ::

        sequence{
            number(8),
            bytes()
        }

.. haka:function:: union(entities) -> entity
    :objtype: grammar
    :module:

    :param entities: List of entities for the union
    :paramtype entities: Table of grammar entities
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Create a union for a list of sub entities. Each entity will be parsed for the
    beginning of the union.

.. haka:function:: try(cases) -> entity

    :param cases: List of grammar entity to try.
    :paramtype cases: Table of grammar entities.
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Parser will try, in order, each case until one of it finishes successfully.

.. haka:function:: branch(cases, selector) -> entity
    :objtype: grammar
    :module:

    :param cases: Branch cases.
    :paramtype cases: Associative table of named grammar entities
    :param selector: Function that will select which case to take.
    :paramtype selector: function
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Create a branch. The case to take will be given by the selector function:

    .. haka:function:: selector(result, context) -> case
        :noindex:
        :module:

        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`
        :return case: The key of the case to select.

    A special case named ``default`` is used as the default branch if none is found. If this
    case is set to the string ``'continue'`` the parsing will continue in the case where no valid
    case is found. If it is not set by the user, a parsing error will be raised.

    **Usage:**

    ::

        branch({
                num8  = number(8),
                num16 = number(16),
            }, function (result, context)
                return result.type
            end
        )

.. haka:function:: optional(entity, present) -> entity
    :objtype: grammar
    :module:

    :param entity: Optional grammar entity.
    :paramtype entity: grammar entity
    :param present: Function that will select if the entity should be present.
    :paramtype present: function
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Create an optional entity. This element exists if the `present` function returns ``true``.

    .. haka:function:: present(result, context) -> is_present
        :noindex:
        :module:

        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`
        :return is_present: True if the element exists.
        :rtype is_present: boolean

.. haka:function:: array(entity) -> entity
    :objtype: grammar
    :module:

    :param entity: Entity representing an element of the array.
    :paramtype entity: grammar entity
    :return entity: Created entity.
    :rtype entity: :haka:class:`GrammarEntity`

    Create an array of a given entity.

    **Supported options:**

    .. haka:data:: count
        :module:
        :idxctx: array
        :objtype: option
        :idxtype: array grammar option

        :type: number

        Number of element in the array.

    .. haka:function:: count(result, context) -> count
        :module:
        :idxctx: array
        :objtype: option
        :idxtype: array grammar option

        :param result: Current parse result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`
        :return count: Number of element in the array.
        :rtype count: number

    .. haka:function:: untilcond(elem, context) -> should_stop
        :module:
        :idxctx: array
        :objtype: option
        :idxtype: array grammar option

        :param elem: Current element of the array. When called before the first element, the parameter is ``nil``.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`
        :return should_stop: Number ``true`` when the end of the array is reached.
        :rtype should_stop: number

    .. haka:function:: whilecond(elem, context) -> should_continue
        :module:
        :idxctx: array
        :objtype: option
        :idxtype: array grammar option

        :param elem: Current element of the array. When called before the first element, the parameter is ``nil``.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext`
        :return should_continue: Number ``false`` when the end of the array is reached.
        :rtype should_continue: number

    **Usage:**

    ::

        array(number(8))
            :options{count = 10}


Converters
----------

.. haka:module:: haka.grammar.converter

.. haka:class:: Converter
    :module:

    A converter allows to apply some processing to a parsing result value.

    .. haka:method:: Converter.get(val)

        Compute the converted value from the raw data. This happens when the user tries
        to get the value of a field for instance.

    .. haka:method:: Converter.set(val)

        Compute the converted value to store in the raw data. This happens when the user
        modify the value of on of the field.

        .. note:: If setter is nil then field will be read-only.

    **Usage:**

    ::

        local my_converter = {
            get = function (val)
                return val:gsub("/", ".")
            end,
            set = nil
        }


Predefined converters
^^^^^^^^^^^^^^^^^^^^^

.. haka:function:: mult(val) -> converter

    :param val: Multiple to apply to the raw value.
    :paramtype val: number
    :return converter: Converter
    :rtype converter: :haka:class:`Converter`

    Create a converter that will apply a multiplication to the raw
    data.

.. haka:data:: bool

    :type: :haka:class:`Converter` |nbsp|

    Convert the raw value into a boolean.

.. haka:function:: tonumber(format, base) -> converter

    :param format: String format to use when converting from number to string.
    :paramtype format: string
    :param base: Base to use for the convertion.
    :paramtype base: number
    :return converter: Converter
    :rtype converter: :haka:class:`Converter`

    Convert a raw string value into a number.


Compiled grammar
----------------

.. haka:currentmodule:: haka.grammar

.. haka:class:: CompiledGrammarEntity
    :module:

    Compiled grammar representation.

    .. haka:method:: CompiledGrammarEntity:parse(iter, result=nil, user=nil) -> result, error

        :param iter: Data iterator.
        :paramtype iter: :haka:class:`vbuffer_iterator`
        :param result: Object where the parsing result will be stored. If `nil`, a generic result object will be created.
        :paramtype result: abstract table
        :param user: User object that will be stored in the parsing context.
        :paramtype user: table
        :return result: The result of the parsing.
        :return error: An error if needed.
        :rtype result: table for the result
        :rtype error: :haka:class:`ParseError`

        Parse the data and store all results in the object returned by the function. In case of error, the error
        desciption is also returned.

    .. haka:method:: CompiledGrammarEntity:create(iter, result=nil, init={}) -> result, error

        :param iter: Data iterator.
        :paramtype iter: :haka:class:`vbuffer_iterator`
        :param result: Object where the parsing result will be stored. If `nil`, a generic result object will be created.
        :paramtype result: abstract table
        :param init: Optional initialization table.
        :paramtype init: table
        :return result: The result of the parsing.
        :return error: An error if needed.
        :rtype result: table for the result
        :rtype error: :haka:class:`ParseError`

        Initialize the data from an initialization table and returned the parsing result. In case of error, the error
        desciption is also returned.


Parsing error
-------------

.. haka:class:: ParseError
    :module:

    Parsing error description.

    .. haka:attribute:: ParseError.iterator

        :type: :haka:class:`vbuffer_iterator` |nbsp|

        Iterator at the position where the parsing error occurred.

    .. haka:attribute:: ParseError.rule

        :type: string

        Name of the rule where the error occurred.

    .. haka:attribute:: ParseError.description

        :type: string

        Full description of the parsing error.


Parsing context
---------------

.. haka:class:: ParseContext
    :module:

    Parsing context used in all parsing related functions.

    .. haka:method:: ParseContext:result(index)

        :param index: Index of the result in the stack.
        :ptype index: number

        Get a parsing result from the stack of results. This stack holds all results
        created during the parsing for records, arrays...

        The index can be a normal index (ie. ``1``
        being the top-level result...) or a pseudo index when it is negative. In this
        case the return value is the result at the position stating from the last
        element. For instance ``-1`` is the last result, ``-2`` is the last but one
        result.

    .. haka:attribute:: ParseContext.user

        User object.

    .. haka:method:: ParseContext:lookahead() -> byte

        :return byte: Next byte.
        :rtype byte: number

        Return the next byte. This function can be used to resolve grammar ambiguity.


Example
-------

This is an example of a very simple grammar expressed in Haka:

::

    local grammar = haka.grammar.new("example", fonction ()
        elem = record{
            field("A", number(32)),
            field("B", number(32))
        }

        block = record{
            field("count", number(32)),
            field("list", array(elem)
                :options{count = function (self)
                    return self.count
                end})
        }

        export(block)
    end)

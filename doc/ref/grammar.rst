.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Grammar
=======

.. haka:module:: haka.grammar

.. haka:function:: new(name) -> grammar

    :param name: Name of the grammar.
    :paramtype name: string
    :return grammar: Created grammar object.
    :rtype grammar: :haka:class:`Grammar` instance

    Create a new grammar. The name is mainly used to report detailed
    information to the user.

.. haka:class:: Grammar
    :module:

    Object holding grammar rules.


Elements
--------

A grammar in Haka is made of various elements that are described in this section. Each one, have different
properties.

.. haka:class:: Entity
    :module:

    Object representing any grammar element. This object have different functions that cas be used in the
    grammar specification

.. haka:method:: Entity:options{...}

    Generic option specification function. Check the documentation of the grammar element to get the list
    of option supported.

    The options are passed as ``key=value`` in the table used as the parameter of the function.

.. haka:method:: Entity:extra{...}

    Method only available on record which can be used to add extra element to it. The table should only
    contains functions.

    Each named element in the array will be added as a extra field in the result.
    The other element will be executed when the record will be done with its parsing.

.. haka:method:: Entity:validate(validator)

    Add a validation function for the element. This function is called when a field is mark invalid by
    setting it to ``nil``.

    .. haka:function:: validator(result)
    
        :param result: Current parsing result.
        :param context: Full parsing context.

.. haka:method:: Entity:convert(converter)

    :param converter: Value converter.
    :paramtype converter: :haka:mod:`haka.grammar.converter`

    Set a conversion operation to apply to the element data.

.. haka:method:: Entity:extra{...}

    Method only available on record which can be used to add extra element to it. The table should only
    contains functions.

    Each named element in the array will be added as a extra field in the result.
    The other element will be executed when the record will be done with its parsing.

.. haka:method:: Entity:compile()

    :rtype: :haka:class:`CompiledEntity` instance

    Compile the grammar representation.


Compounds
^^^^^^^^^

.. haka:function:: record(entities) -> entity

    :param entities: List of entities for the record
    :paramtype entities: table of grammar entities
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Create a record for a list of sub entities. Each entity is expected to appear
    one by one in order.

    Usage::

        haka.grammar.record{
        	haka.grammar.number(8),
        	haka.grammar.bytes()
        }

.. haka:function:: union(entities) -> entity

    :param entities: List of entities for the union
    :paramtype entities: Table of grammar entities
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Create a union for a list of sub entities. Each entity will be parsed for the
    beginning of the union.

.. haka:function:: branch(cases, selector) -> entity

    :param cases: Branch cases.
    :paramtype cases: associative table of named grammar entities
    :param selector: Function that will select which case to take.
    :paramtype selector: function
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Create a branch. The case to take will be given by the selector function:

    .. haka:function:: selector(result, context) -> case
    
        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext` instance
        :return case: The key of the case to select.

    A special case named ``default`` is used as the default branch if none is found. If this
    case is set to the string ``'continue'`` the parsing will continue in the case where no valid
    case is found. If it is not set by the user, a parsing error will be raised.

    Usage::

        haka.grammar.branch({
        		num8  = haka.grammar.number(8),
        		num16 = haka.grammar.number(16),
        	}, function (result, context)
        		return result.type
        	end
        )

.. haka:function:: optional(entity, present) -> entity

    :param entity: Optional grammar entity.
    :paramtype entity: grammar entity
    :param present: Function that will select if the entity should be present.
    :paramtype present: function
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Create an optional entity. This element exists if the `present` function returns ``true``.

    .. haka:function:: present(result, context) -> is_present
    
        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext` instance
        :return is_present: True if the element exists.
        :rtype is_present: boolean

.. haka:function:: array(entity) -> entity

    :param entity: Entity representing an element of the array.
    :paramtype entity: grammar entity
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

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
        :paramtype context: :haka:class:`ParseContext` instance
        :return count: Number of element in the array.
        :rtype count: number

    .. haka:function:: untilcond(elem, context) -> should_stop
        :module:
        :idxctx: array
        :objtype: option
        :idxtype: array grammar option

        :param elem: Current element of the array. When called before the first element, the parameter is ``nil``.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext` instance
        :return should_stop: Number ``true`` when the end of the array is reached.
        :rtype should_stop: number

    .. haka:function:: whilecond(elem, context) -> should_continue
        :module:
        :idxctx: array
        :objtype: option
        :idxtype: array grammar option

        :param elem: Current element of the array. When called before the first element, the parameter is ``nil``.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext` instance
        :return should_continue: Number ``false`` when the end of the array is reached.
        :rtype should_continue: number

    Usage::

        haka.grammar.array(haka.grammar.number(8))
        	:options{count = 10}


Final elements
^^^^^^^^^^^^^^

.. haka:function:: number(bits) -> entity

    :param bits: Size of the number in bits.
    :paramtype bits: number
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    **Supported options:**

    .. haka:data:: endianness
        :module:
        :idxctx: number
        :objtype: option
        :idxtype: number grammar option

        Endianness of the raw data: ``little`` or ``big``. By default, the data will be treated
        as big endian.

    Usage::

        haka.grammar.number(8)

    Parse a binary number.

.. haka:function:: token(pattern) -> entity

    :param pattern: Regular expression pattern for the token.
    :paramtype pattern: size
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Match a regular expression on the data.

    Usage::

        haka.grammar.token('%s+')

.. haka:data:: flag

    :type: :haka:class:`Entity` instance

    Parse a flag of 1 bit and returns it as a ``boolean``.

.. haka:function:: bytes() -> entity

    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

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
        :paramtype context: :haka:class:`ParseContext` instance
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
        :paramtype context: :haka:class:`ParseContext` instance

        This option allows to get each data as soon as they are received in a callback function.

.. haka:function:: padding{align=align_bit} -> entity
                   padding{size=size_bit} -> entity

    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Parse some padding. The padding can be given by size or by alignment.

.. haka:function:: field(name, entity) -> entity

    :param name: Name of the field in the result.
    :paramtype name: string
    :param entity: Entity to named.
    :paramtype entity: grammar entity
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Create a named entity. This is used to give access to an entity of the grammar. It
    will then be possible to access to data in the result in a security rule for instance.

    Usage::

        haka.grammar.field("WS", haka.grammar.token('%s+'))

.. haka:function:: verify(verif, msg) -> entity

    :param verif: Verification function.
    :paramtype verif: function
    :param msg: Error message to report.
    :paramtype msg: string
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Verify some property during the parsing. If ``func`` returns ``false``, then an error is
    reported with ``msg``.

    .. haka:function:: verif(result, context) -> is_valid
    
        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext` instance
        :return is_valid: False if the verification fails.
        :rtype is_valid: boolean

.. haka:function:: execute(exec) -> entity

    :param exec: Generic function.
    :paramtype exec: function
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    Execute a generic function during the parsing. This allows to deeply customize the parsing using
    regular Lua functions.

    .. haka:function:: exec(result, context)
    
        :param result: Current parsing result.
        :param context: Full parsing context.
        :paramtype context: :haka:class:`ParseContext` instance

.. haka:function:: retain(readonly = false) -> entity

    :param readonly: True if the retain should only be read-only.
    :paramtype readonly: boolean
    :return entity: Created entity.
    :rtype entity: :haka:class:`Entity` instance

    When working on a stream, it is needed to specify which part of the stream to keep before being able
    to send it on the network. This element allows to control it.

    .. seealso:: :haka:data:`release`

.. haka:data:: release

    :type: :haka:class:`Entity` instance

    When working on a stream, this element will tell Haka to send some retained data.

    .. seealso:: :haka:func:`retain`


Converters
----------

.. haka:module:: haka.grammar.converter

.. haka:class:: Converter
    :module:

    A converter allows to apply some processing to a parsing result value.

.. haka:function:: Converter.get(val)

    Compute the converted value from the raw data. This happens when the user tries
    to get the value of a field for instance.

.. haka:function:: Converter.set(val)

    Compute the converted value to store in the raw data. This happens when the user
    modify the value of on of the field.


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

    :type: :haka:class:`Converter`

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

.. haka:class:: CompiledEntity
    :module:

    Compiled grammar representation.

.. haka:method:: CompiledEntity:parse(iter, result=nil, user=nil) -> result, error

    :param iter: Data iterator.
    :paramtype iter: :haka:class:`haka.vbuffer_iterator` or :haka:class:`haka.vbuffer_iterator_blocking` instance
    :param result: Object where the parsing result will be stored. If `nil`, a generic result object will be created.
    :paramtype result: abstract table
    :param user: User object that will be stored in the parsing context.
    :paramtype user: table
    :return result: The result of the parsing.
    :return error: An error if needed.
    :rtype result: table for the result
    :rtype error: :haka:class:`ParseError` instance

    Parse the data and store all results in the object returned by the function. In case of error, the error
    desciption is also returned.

.. haka:method:: CompiledEntity:create(iter, result=nil, init={}) -> result, error

    :param iter: Data iterator.
    :paramtype iter: :haka:class:`haka.vbuffer_iterator` instance
    :param result: Object where the parsing result will be stored. If `nil`, a generic result object will be created.
    :paramtype result: abstract table
    :param init: Optional initialization table.
    :paramtype init: table
    :return result: The result of the parsing.
    :return error: An error if needed.
    :rtype result: table for the result
    :rtype error: :haka:class:`ParseError` instance

    Initialize the data from an initialization table and returned the parsing result. In case of error, the error
    desciption is also returned.


Parsing error
-------------

.. haka:class:: ParseError
    :module:

    Parsing error description.

.. haka:attribute:: ParseError.iterator

    :type: :haka:class:`haka.vbuffer_iterator` instance

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

.. haka:attribute:: ParseContext.result

    Current parsing result.

.. haka:attribute:: ParseContext.top

    Top level parsing result.

.. haka:attribute:: ParseContext.prev_result

    Previous level parsing result.

.. haka:attribute:: ParseContext.user

    User object.

.. haka:method:: ParseContext:lookahead()

    :rtype: number

    Return the next byte. This function can be used to remove grammar ambiguity.

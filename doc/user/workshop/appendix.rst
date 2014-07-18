.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Appendix
========

Workshop media
--------------

A media is provided which contains all you need to do the Haka workshop.
You can download it from our web-site in the section *Resources*.

This media contains:

* Haka Debian packages for various architecture (located in `/haka`).
* Haka full documentation (in `haka/manual`) containing the workshop.
* A full bootable live system contains all dependencies of Haka (usable
  in a virtual machine (VirtualBox for instance) or directly as a bootable
  USB or CD.

Tips
^^^^

Accessing media in the live system
""""""""""""""""""""""""""""""""""

When booting on the live system, the media is mounted at `/lib/live/mount/medium`.
A link on the desktop is provided pointing to the workshop page.

Default user/password
"""""""""""""""""""""

The default user of the live system is ``user`` and password ``live``. This is true
for both the main system and also the client and server virtual machines.

Changing keyboard mapping
"""""""""""""""""""""""""

On the console::

    $ sudo dpkg-reconfigure keyboard-configuration
    $ sudo service keyboard-setup restart

Inside X11::

    $ setxkbmap fr

Lua
---

Lua is a simple, lightweight and powerful language. It is used inside
Haka to define configurations.

The full documentation is available at http://www.lua.org/.

Some basic tutorial to introduce Lua are also available at
http://lua-users.org/wiki/TutorialDirectory.

You can do some tests with the lua interpreter :

.. ansi-block::
    :string_escape:

    $ lua
    Lua 5.2.1  Copyright (C) 1994-2012 Lua.org, PUC-Rio
    >

The following is aimed at giving a quick overview of Lua language.

Comments
^^^^^^^^

Lua one-line comments are done with a ``--``:

.. code-block:: lua

    -- This is a comment

Variables
^^^^^^^^^

Lua variables are global except if they are preceded with ``local`` keyword.
It is possible to declare it anywhere.

.. code-block:: lua

    local mylocalvar = "foo"
    myglobalvar = "bar"

.. note:: It is strongly discouraged to use global variables.

Types in Lua
^^^^^^^^^^^^

Basic types
"""""""""""

Lua define the following basic types:

    * `nil` - represent a nil value. It is also a keyword.
    * `boolean` - represent a boolean value. Can be ``true`` or ``false``.
    * `number` - represent a number. Can be either an integer or a floating
      point number.
    * `string` - represent a character string.
    * `table` - represent a table of object.
    * `function` - represent a function.
    * `userdata` - a complex type used to map C data.

String
""""""

Lua allow to easily concatenate string with the ``..`` keyword.

.. code-block:: lua

    foo = "foo"
    bar = "bar"
    print(foo.." : "..bar)

To format a string, the Lua library ``string`` provides an utility function
to do this.

.. code-block:: lua

    print(string.format("%s: %d", "foo", 42)

Tables
""""""

As tables are used almost everywhere in Lua (and obviously in haka) you should
know a few things about it. It can be used to represent an array or an object.

Lua table declaration is made with ``{}``.

When used as an array, Lua table index starts at ``1``.

.. code-block:: lua

    mytable = { "foo" }
    print(mytable[1])

Getting a inexistent element of a table return ``nil``. It is then not an
error to access an element not present.

.. code-block:: lua

    mytable = { "foo" }
    print(mytable[2])

Setting an element in a table to ``nil`` simply remove it from the table.

.. code-block:: lua

    mytable = { "foo", "bar" }
    mytable[1] = nil
    print(mytable[1])
    print(mytable[2])

.. note:: Lua does not reorder the table when an element is removed from it.
    To do this, check the Lua library `table`.

Lua tables can be used as a map. Namely you can index element of a table with
any type of value.

Declaration of such element are done with a simple ``=`` in declaration or by
adding it a posteriori:

.. code-block:: lua

    mytable = { foo = "myfoostring" }
    mytable.bar = "mybarstring"

Access to this element can be done with the usual ``[]`` or directly with a
``.``.

.. code-block:: lua

    print(mytable["foo"])
    print(mytable.bar)

.. note:: Don't forget that accessing an inexistent element of a table return
    ``nil``. Even for a table used as a map.

It is possible to mix indexed table and mapped table.

.. code-block:: lua

    mytable = { "foo" }
    mytable.bar = "bar"

    print(mytable[1])
    print(mytable.bar)

Lua provide two `iterator` function to loop on table.

First one is called ``ipairs()`` and it will loop over indexed value of the
table.

Second one is called ``pairs()`` and it will loop over every value of the table.

.. seealso:: :ref:`for-loop-statement` for more information on how to use
    ``pairs()`` and ``ipairs()``.


Boolean logic
^^^^^^^^^^^^^

Lua offers the usual boolean operators:

    * ``and``
    * ``or``
    * ``not``

.. warning:: In Lua, everything is ``true`` except ``nil`` and ``false``. For
    instance ``0`` is ``true``.

Control flow
^^^^^^^^^^^^

if-then-(else)
""""""""""""""

Lua ``if-then-(else)`` statement looks like:

.. code-block:: lua

    if condition then
        -- [...]
    else
        -- [...]
    end

.. note:: It is not required to wrap conditions inside parenthesis.

.. _for-loop-statement:

for loops
"""""""""

Lua ``for`` loop statement looks like:

.. code-block:: lua

    for i = first,last,delta do
        -- [...]
    end

.. note:: ``delta`` may be negative, allowing the for loop to count down or up.

Lua have a ``for-in`` loop statement:

.. code-block:: lua

    for _, element in pairs(mytable) do
        -- [...]
    end

A ``for-in`` statement use the special ``in`` keyword followed by a call to
one of ``pairs()`` or ``ipairs()`` functions. Both return two values: ``index,
value``.

.. note:: Lua recommends the use of ``_`` for unused variable

while and repeat-until loops
""""""""""""""""""""""""""""

Lua have a ``while`` loop statement:

.. code-block:: lua

    while condition do
        -- [...]
    end

Lua provide an opposite loop statement called ``repeat-until``:

.. code-block:: lua

    repeat
        -- [...]
    until condition

Functions
^^^^^^^^^

Lua functions are declared with the ``function`` keyword.

.. code-block:: lua

    function myfunc()
        -- [...]
    end

Lua function can return multiple values.

.. code-block:: lua

    function myfunc()
        return true, 1, "foo"
    end

    ok, count, bar = myfunc()

There is not error when calling a function with an incorrect number
of arguments. The extra arguments are not used and the missing one
are set to ``nil``.

.. code-block:: lua

    function myfunc(a, b, c)
        print(a, b, c)
    end

    myfunc(10) -- b and c will be equal to nil

Function are first-class value in Lua. This means that they can be used exactly
like a ``number`` for instance. As an example, it is possible to return a function
from a function.

.. code-block:: lua

    function myfunc(a)
        return function ()
            print(a)
        end
    end

    myfunc(10)()


.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Contribute
==========

You are welcome to contribute to this project. You can follow us on GitHub:
https://github.com/haka-security/haka.

Coding conventions (for C)
--------------------------

Indentation
^^^^^^^^^^^

Use good indentation.

Indent with tabs. The tab size can be set by each user to its preferred value. This means
that tabs should not be used to align anything but only to indent. If you need to align,
use spaces.

Try to maintain line size bellow 80 for tab size of 4.

Brackets
^^^^^^^^

Brackets are placed at the end of the line after a `if`, `for`... One exception is made for the
function where the open bracket is placed on the next line.

Closing brackets are placed at the beginning of the line. It is possible to put it on the same line
in case of `then`.

If a code block contains only one instruction, this block should use brackets or should be put on the
same line to avoid errors.

::

    void function (void)
    {
        if (cond) {
            action;
        } else if (cond) {
            action;
        }
        else {
            action;
        }

        do {
            action;
        } while (cond);

        if (cond) action;
    }

Spaces
^^^^^^

For the function calls, no space after the name of the function and each parameter is separated by a
comma and a space.

::

    function(param1, param2, param3);

In a expression between parenthesis, add a space before it.

::

    if (condition)
    if (param == 0)
    if (!param)
    if (table[elt] == NULL || table[elt] != 42)
    if ((p1 == 2) && (p2 == 5))


Modules
^^^^^^^

Use good modular design and good error detection.

Comments
^^^^^^^^

Use C comments `/* .. */`.


Git branches
------------

Haka uses the Git flow model to manage its branches. Here is a quick definition of the
various branches.

* *master*: Main branch that contains the current release of Haka.
* *develop*: Current developments.
* *release/vX.Y*: Release branch that will only exists until the release is done where it is merged to *master*.
* *feature/x*: Feature branches that will be merged to *develop* when completed.

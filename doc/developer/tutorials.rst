.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Writing Haka extensions
=======================

Haka has a modular architecture and you can extend it by adding your own
module like a new packet capture module.

There is multiple way to embed your favorite module in Haka. We provide here a
Makefile to compile your source files, link your module against haka libraries
and have it installed on haka target path.

In the following, we will create a sample module ``mymodule`` that exports a
single function ``myfunc``.

First, create a directory ``mymodule`` and two sub folders ``src`` and ``obj``.
In the former, create your source and header files.

The source file ``mymodule.c`` must include a module structure that sets the
type of the module to ``MODULE EXTENSION`` along with other information such as
the name and the author of the module:

.. code-block:: c

    struct module MY_MODULE = {
        type:        MODULE_EXTENSION,
        name:        L"my module",
        description: L"my module",
        api_version: HAKA_API_VERSION,
        init:        init,
        cleanup:     cleanup
    };

The module code must also defines a initialization and a cleanup function that
will be called when the module loads and unloads, respectively.

.. code-block:: c

    static int init(struct parameters args)//
    {
        messagef(HAKA_LOG_INFO, L"mymodule", L"init my module");
        return 0;
    }

    static void cleanup()
    {
        messagef(HAKA_LOG_INFO, L"mymodule", L"cleanup my module");
    }

Here is the :download:`mymodule.c <../../sample/mymodule/src/mymodule.c>` which defines
also the ``myfunc`` function that we want to export.

Then, we have created a single header file :download:`mymodule.h <../../sample/mymodule/src/mymodule.h>`
That simply defines the prototype of the function ``myfunc``.

Finally, in Haka we use `SWIG <http://www.swig.org/Doc1.3/Lua.html>`_ to bind C
methods. Here is the content of our swig file that binds the ``myfunc`` function.

.. note:: for more complex binding, you can have a look at C protocol modules in
    Haka source tree.

The last step is to compile (``make``) our newly created module using the following
:download:`Makefile <../../sample/mymodule/Makefile>` that must be created in the main module folder.

.. literalinclude:: ../../sample/mymodule/Makefile
    :tab-width: 4
    :language: makefile

.. note:: You may need to adjust the HAKA_PATH variable.

Now, we are ready to use the module in haka script files. Here is a sample file
that simply calls the ``myfunc`` function:

.. literalinclude:: ../../sample/mymodule/rule.lua
    :tab-width: 4
    :language: lua

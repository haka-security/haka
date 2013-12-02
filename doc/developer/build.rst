
Build
=====

.. note::

    The installation instructions are for Debian. It will need
    to be adapted to other platform.

Dependencies
------------

Required
^^^^^^^^

* Toolchain (GCC, Make, ...)
* cmake
* swig
* python-sphinx
* liblua5.1
* tshark
* check
* rsync
* libpcap-dev
* gawk
* libedit-dev

Debian:

.. code-block:: console

    $ sudo apt-get install build-essential cmake swig python-sphinx liblua5.1 tshark check rsync libpcap-dev gawk libedit-dev

Optional
^^^^^^^^

* Cppcheck
* Netfilter Queue
* Valgrind

Debian:

.. code-block:: console

    $ sudo apt-get install cppcheck libnetfilter-queue-dev valgrind

Build
-----

Submodules
^^^^^^^^^^

The repository uses submodules that need to be initialized and updated:

.. code-block:: console

    $ git submodule init
    $ git submodule update

Configure
^^^^^^^^^

It is mandatory to create a separate directory to store
all the files generated during the build using cmake.

.. code-block:: console

    $ mkdir make
    $ cd make
    $ cmake .. <options>

Options
"""""""

To add an option to cmake, add ``-DOPTION=VALUE`` to the command line option when calling cmake.
The configuration with cmake supports the following options:

.. option:: BUILD=[Debug|Memcheck|Release|RelWithDebInfo|MinSizeRel]

    Select the build type to be compiled (default: *Release*)

.. option:: LUA=[lua51|luajit]

    Choose the Lua version to use (default: *luajit*)

.. option:: PREFIX=PATH

    Installation prefix (default: */*)

Compile
^^^^^^^

Use make like usual to compile:

.. code-block:: console

    $ make clean
    $ make

Install
^^^^^^^

To install haka, you have the following options:

.. code-block:: console

    $ make install
    $ make localinstall

Using ``localinstall`` allow to install haka locally under the folder ``out``. To use
this version, you will have to set a few environment variables by sourcing the generated
file ``env.sh``:

.. code-block:: console

    $ . env.sh

Documentation
^^^^^^^^^^^^^

Run ``make doc`` to generate documentation in `html`. The documentation is then available
in `doc` inside your build folder.

Tests
^^^^^

Run ``make tests`` to play all tests.

You can also pass some arguments to ctest by using the variable ``CTEST_ARGS``.

.. code-block:: console

    $ make tests CTEST_ARGS="-V"

This command will install locally the project and run the tests in the folder. If you need
to run the tests manually using the command ctest, you can prepare the environment with the
command ``make pre-tests``.

Packaging
^^^^^^^^^

Run ``make package`` to build a .deb installable package.

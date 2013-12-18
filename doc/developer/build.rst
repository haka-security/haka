.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Build
=====

.. note::

    The build instructions are for Debian. It will need to be adapted to
    other platform.

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

Git
^^^

You must first clone the Git repository. Our project is hosted on GitHub:

.. code-block:: console

    $ git clone git@github.com:haka-security/haka.git

Our development uses the branching model Git flow which describe how to
use and name Git branches. For instance, you will find the following branches:

* ``master`` branch contains the last release of Haka. This branch might be empty
  if we do not have an official version.
* ``develop`` branch contains the current Haka unstable development.

You should then switch to the branch you want to build. For example:

.. code-block:: console

    $ git checkout develop

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

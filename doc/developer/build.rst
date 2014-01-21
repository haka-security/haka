.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Build
=====

Dependencies
------------

Required
^^^^^^^^

* Toolchain (GCC, Make, ...)
* cmake (>= 2.8)
* swig
* sphinx (>= 2)
* tshark
* check
* rsync
* libpcap
* gawk
* libedit

Optional
^^^^^^^^

* Git
* Cppcheck
* Netfilter Queue
* Valgrind
* rpm-build

Examples
^^^^^^^^

Debian (and compatible)
"""""""""""""""""""""""

.. code-block:: console

    $ sudo apt-get install build-essential cmake swig python-sphinx tshark check
    $ sudo apt-get install rsync libpcap-dev gawk libedit-dev
    $ sudo apt-get install cppcheck libnetfilter-queue-dev valgrind

Fedora
""""""

.. code-block:: console

    $ sudo yum install gcc gcc-c++ make cmake python-sphinx wireshark check
    $ sudo yum install check-devel rsync libpcap-devel gawk libedit-devel
    $ sudo yum install git cppcheck libnetfilter_queue-devel rpm-build valgrind valgrind-devel

The *swig* package in Fedora is broken and will not be usable to compile Haka.
You will need to get a swig build from upstream.

.. code-block:: console

    $ git clone https://github.com/swig/swig.git
    $ cd swig
    $ git co rel-2.0.11
    $ ./autogen.sh
    $ ./configure
    $ make
    $ sudo make install

Download
--------

You can get the sources of Haka from a tarball or directly by cloning the Git
repository.

From sources tarball
^^^^^^^^^^^^^^^^^^^^

First download the source tarball from the
`Haka website <http://www.haka-security.org>`_.

Then type the following commands:

.. code-block:: console

    $ tar -xzf haka.tar.gz
    $ cd haka

Git
^^^

You must first clone the Git repository. Our project is hosted on GitHub:

.. code-block:: console

    $ git clone git@github.com:haka-security/haka.git

Our development uses the branching model Git flow which describes how to
use and name Git branches. For instance, you will find the following branches:

* ``master`` branch contains the last release of Haka. This branch might be empty
  if we do not have an official version.
* ``develop`` branch contains the current Haka unstable development.

You should then switch to the branch you want to build. For example:

.. code-block:: console

    $ git checkout develop

Submodules
""""""""""

The repository uses submodules that need to be initialized and updated:

.. code-block:: console

    $ git submodule init
    $ git submodule update

Build
-----

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

To install Haka on your system, type this command:

.. code-block:: console

    $ sudo make install

By default, Haka will be installed in ``/opt/haka``. You might want to update your ``PATH``
environment variable to be able to easily launch the various tools from the command line.

Local install
"""""""""""""

To install Haka locally, type this command:

.. code-block:: console

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

Run ``make package`` to build an installable package.

.. note::

    If you have some issue with the folder permission in the generated package, check your
    umask property. If you hit this problem, for instance, rpm will complains about conflicting
    directory.

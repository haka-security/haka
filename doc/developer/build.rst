.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Build
=====

Dependencies
------------

Required
^^^^^^^^

* Classic build tools (GCC, Make, ...)
* CMake (>= 2.8)
* Swig
* Tshark
* Check
* rsync
* libpcap
* Gawk
* libedit
* libpcre

Optional
^^^^^^^^

* Git
* Cppcheck
* Netfilter Queue
* Valgrind
* rpm-build
* Sphinx (>= 2)
* Doxygen
* Inkscape
* python-blockdiag
* python-seqdiag


Examples
^^^^^^^^

Debian (and compatible)
"""""""""""""""""""""""

.. code-block:: console

    $ sudo apt-get install build-essential cmake swig tshark check
    $ sudo apt-get install rsync libpcap-dev gawk libedit-dev libpcre3-dev
    $ sudo apt-get install cppcheck libnetfilter-queue-dev valgrind
    $ sudo apt-get install python-sphinx doxygen python-blockdiag python-seqdiag

Fedora
""""""

.. code-block:: console

    $ sudo yum install gcc gcc-c++ make cmake python-sphinx wireshark check doxygen
    $ sudo yum install check-devel rsync libpcap-devel gawk libedit-devel pcre-devel
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

    $ git clone https://github.com/haka-security/haka.git

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

    $ mkdir workspace
    $ cd workspace
    $ cmake .. <options>

Options
"""""""

To add an option to cmake, add ``-DOPTION=VALUE`` to the command line option when calling cmake.
The configuration with cmake supports the following options:

.. option:: BUILD=[Debug|Memcheck|Release|RelWithDebInfo|MinSizeRel]

    Select the build type to be compiled (default: *Release*)

.. option:: LUA=[lua|luajit]

    Choose the Lua version to use (default: *luajit*)

.. option:: PREFIX=PATH

    Installation prefix (default: */*)

Compile
^^^^^^^

Use make like usual to compile:

.. code-block:: console

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

    $ cd out/
    $ . env.sh

Documentation
^^^^^^^^^^^^^

Run ``make doc`` to generate documentation in `html`. The documentation is then available
in `doc` inside your build folder.

In order to build it, you need to have, at least, Sphinx and Doxygen installed. To
get all images, you also need the tools Inkscape, blockdiag and seqdiag. You might need to
install the fonts used for those images in your system. The files are located in
`doc/theme/haka/fonts`.

Tests
^^^^^

Run ``make tests`` to play all tests.

You can also pass some arguments to ctest by using the variable ``CTEST_ARGS``.

.. code-block:: console

    $ make tests CTEST_ARGS="-V"

This command will install locally the project and run the tests in the folder. If you need
to run the tests manually using the command ctest, you can prepare the environment with the
command ``make pretests``.

Packaging
^^^^^^^^^

Run ``make package`` to build a tgz package.

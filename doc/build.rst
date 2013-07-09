
Build
=====

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

Debian: ::

    sudo apt-get install build-essential cmake swig python-sphinx liblua5.1 tshark check rsync libpcap-dev gawk libedit-dev

Optional
^^^^^^^^

* Cppcheck
* Netfilter Queue

Debian: ::

    sudo apt-get install cppcheck libnetfilter-queue-dev

Build
-----

Configure
^^^^^^^^^

It is highly recommanded to create a separate directory to store
all the files generated during the build using cmake. ::

    mkdir make
    cd make
    cmake ..

Options
"""""""

To add an option to cmake, add ``-DOPTION=VALUE`` to the command line option when calling cmake.
The configuration with cmake supports the following options:

.. option:: BUILD=[Debug|Memcheck|Release|RelWithDebInfo|MinSizeRel]

    Select the build type to be compiled (default: *Release*)

.. option:: USE_LUAJIT=[YES|NO]

    Compile haka with LuaJit or with the standard Lua library (default: *YES*)

.. option:: PREFIX=PATH

    Installation prefix (default: *out*)

Compile
^^^^^^^

The repository uses submodules that need to be initialized and updated: ::

    git submodule init
    git submodule update

Then use make like usual to compile: ::

    make
    make install
    make clean

Documentation
^^^^^^^^^^^^^

Run ``make doc`` to generate documentation in `html`. The documentation is then available
in `doc` inside your build folder.

Tests
^^^^^

Run ``make test`` to play some unit tests.

Packaging
^^^^^^^^^

Run ``make package`` to build an installable package.

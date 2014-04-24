
HAKA Runtime
============

What is HAKA
------------

Haka is a collection of tool that allows capturing TCP/IP packets and filtering
them based on Lua policy files.

Dependencies
------------

### Required

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
* libpcre

### Optional

* Git
* Cppcheck
* Netfilter Queue
* Valgrind

Submodules
----------

If using the Git repository, it uses submodules that need to
be initialized and updated:

    $ git submodule init
    $ git submodule update

Build
-----

### Configure

It is required to create a separate directory to store
all the files generated during the build using cmake.

    $ mkdir make
    $ cd make
    $ cmake ..

### Build

Then use make like usual to compile (`make`) and install (`make install`) or
clean (`make clean`).

    $ make
    $ sudo make install

### Documentation

Run `make doc` to generate documentation in `html`. The documentation is then available
in `doc` inside your build folder.

The documentation contains more information about building and using Haka.

License
-------

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

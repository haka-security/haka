HAKA Runtime
============

Dependencies
------------

### Required

* Toolchain (GCC, Make, ...)
* cmake
* swig
* sphinx
* liblua5.1
* tshark
* check
* rsync
* libpcap-dev
* gawk

#### Debian

    $ sudo apt-get install build-essential cmake swig sphinx liblua5.1 tshark check rsync libpcap-dev gawk

### Optional

* Cppcheck
* Netfilter Queue

#### Debian

    $ sudo apt-get install cppcheck libnetfilter-queue-dev

Build
-----

### Configure

It is highly recommanded to create a separate directory to store
all the files generated during the build using cmake.

    $ mkdir make
    $ cd make
    $ cmake ..

### Build

The repository uses submodules that need to be initialized and updated:

    $ git submodule init
    $ git submodule update

Then use make like usual to compile (`make`) and install (`make install`) or
clean (`make clean`).

    $ make
    $ make install
    $ make clean

### Documentation

Run `make doc` to generate documentation in `html`. The documentation is then available
in `doc` inside your build folder.

The documentation contains more information about building and using Haka.

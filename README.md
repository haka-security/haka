HAKA Runtime
============

Haka-Runtime is a tool that allows to capture TCP/IP packet and filter
them based on Lua policy files.

Major features:

  - Capture/Drop packet using nfqueue 
  - Read packet either from a pcap file or from interface using libpcap
  - Filter packet from Lua policy files using wireshak-like expressions
  - Log using either stdout or syslog modules
  - Provide accessors to TCP and IP headers fields
  - Forge TCP and IP headers packet
  - And more features coming soon...


Authors / Contributors
----------------------

### Authors:
  - Kevin Denis
  - Pierre-Sylvain Desse
  - Mehdi Talbi
  - Jeremy Rosen

### Contributors:
  - Remi Bauzac


Dependencies
------------

### Required

  - Toolchain (GCC, Make, ...)
  - cmake
  - swig
  - doxygen
  - liblua5.1
  - tshark
  - check
  - rsync
  - libpcap-dev

#### Debian

    $ sudo apt-get install build-essential cmake swig doxygen liblua5.1 tshark check rsync libpcap-dev

### Optional

  - Cppcheck (optional)
  - Netfilter Queue (optional)

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

#### Options

To add an option to cmake, add `-DOPTION=VALUE` to the command line option when calling cmake.
The configuration with cmake supports the following options:

  - `CMAKE_BUILD_TYPE`=[Debug|Release|RelWithDebInfo|MinSizeRel]: Select the build type to be
  compiled (default: Release)
  - `USE_LUAJIT`=[YES|NO]: Compile haka with LuaJit or with the standard Lua library (default: YES)

### Build

The repository uses submodules that need to be initialized and updated:

    $ git submodule init
    $ git submodule update

Then use make like usual to compile (`make`) and install (`make install`) or
clean (`make clean`).

    $ make
    $ make install
    $ make clean

### Tests

Run `make test` to play some unit tests.

### Documentation

Run `make doc` to generate doxygen documentation in `html` and `latex` format.
The documentation is available in `Documentation` inside your build folder.

### Packaging

Run `make package` to get a tar.gz archive of the whole project.


Usage
-----
_TODO_


Files
-----

  - src: haka core application source files
  - include: header files shared between haka application and haka modules
  - modules: module source files
  - lib: shared libraries
  - external: external dependencies
  - build: cmake scripts for the build


Bugs/Patches/Contact
--------------------
_TODO_


License
-------
_TODO_

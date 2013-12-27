.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Installation
############

.. note::

    The installation instructions are for Debian. It will need to be adapted to
    other distributions.

Install from package
====================

Download the package
--------------------

First download the Debian package from the Haka website http://www.haka-security.org.

Then to install it, type the following command:

.. code-block:: console

    $ sudo dpkg -i haka.deb


Install from source
===================

Pre-installation requirements
-----------------------------

To build and install Haka you will need to install some required packages.

Enter the following command to install them (this make take some time):

.. code-block:: console

    $ sudo apt-get install build-essential cmake swig python-sphinx \
    tshark check rsync libpcap-dev gawk libedit-dev libnetfilter-queue-dev

Download the sources
--------------------

First download the source tarball from the Haka website http://www.haka-security.org.

Then type the following commands:

.. code-block:: console

    $ tar -xvzf haka.tar.gz
    $ cd haka

Compile and install Haka
------------------------

.. code-block:: console

    $ mkdir make
    $ cd make
    $ cmake ..
    $ make
    $ sudo make install

By default, Haka will be installed in ``/opt/haka``. You might want to update your ``PATH``
environment variable to be able to easily launch the various tools from the command line.

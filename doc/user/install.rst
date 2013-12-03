
Installation
############

.. note::

    The installation instructions are for Debian. It will need to be adapted to
    other platform.

Install from package
====================

Download the package
--------------------

To download and install the Haka package, enter the following:

.. code-block:: console

    $ wget http://www.haka-security.org/download/haka-0.1.deb
    $ sudo dpkg -i haka-0.1.deb


Install from source
===================

Pre-installation requirements
-----------------------------

Some packages are required to build and run Haka. This operation make take some time.

.. code-block:: console

    $ sudo apt-get install build-essential cmake swig python-sphinx liblua5.1 \
    tshark check rsync libpcap-dev gawk libedit-dev libnetfilter-queue-dev

Download the sources
--------------------

To download the Haka sources, enter the following:

.. code-block:: console

    $ wget http://www.haka-security.org/download/haka-0.1.tar.gz
    $ tar -xvzf haka-0.1.tar.gz
    $ cd haka

Compile and install Haka
------------------------

.. code-block:: console

    $ mkdir make
    $ cd make
    $ cmake ..
    $ make
    $ sudo make install

Haka will be installed by default in ``/opt/haka``. You might want to update your ``PATH``
environment variable in order to simplify Haka starting tools.

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Statistics
==========

Introduction
------------
This tutorial shows how to collect traffic and make statistics on collected data.

How-to
------
This tutorial introduces two Haka script files: ``stats_on_exit`` and ``stats_interactive`` which could be ran using the ``hakapcap`` tool as follows:

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/stats
    $ hakapcap <pcap_file> <script_stat_file>

.. note::

    In this tutorial we use a pre-processed pcap file originated from the
    DARPA dataset and which could be retrieved from the `MIT website <http://www.ll.mit.edu/mission/communications/cyber/CSTcorpora/ideval/data/>`_.
    We filtered out some packets to get a reasonable size capture that
    you can download from the `Haka website <http://www.haka-security.org/>`_
    in the *Resources* section.

Collecting data
---------------
Before making statisctics, we need first to collect data. This is the purpose of ``stats.lua`` file which starts by creating a global `stats` table. This table will be updated with http info whenever a new http response is received. An entry in the `stats` table consists of the following fields:

- ip: source ip
- method: http request method (`get`, `post`, etc.)
- host: http host
- resource: normalized path
- referer: referer header
- usergant: user-agent header
- status: response status code

.. note::

    While the security rule is evaluated whenever a http response is received,
    we still get access to http request fields and ip header fields.

.. literalinclude:: ../../../sample/stats/stats.lua
    :tab-width: 4
    :language: lua

Stats utilities
----------------

This section introduces the stats utilities developed for this tutorial. More
precisely, it shows how to create the global `stats` table and how to run basic
stats operations on the created table.

.. haka:module:: stats_utils

.. haka:function:: new() -> table

    :return table: New stats table.
    :rtype table: :haka:class:`stats`

    Create the stats table.

.. haka:class:: stats

    .. haka:method:: stats:list()

        Print column names of stats table.

    .. haka:method:: stats:dump([nb])

        :param nb: Number of entries to display.
        :ptype nb: number

        Print *nb* entries of stats table.

    .. haka:method:: stats:top(column_name[, nb])

        :param column_name: Column to query.
        :ptype column_name: string
        :param nb: Number of entries to display.
        :ptype nb: number

        Dump the top 10 of given field name. Limits output to *nb* if it is provided.

    .. haka:method:: stats:select_table(column_tab[, where])

        :param column_name: Column to query.
        :ptype column_name: string
        :param where: Filter function called for each table line.
        :ptype where: function

        Select specific columns from table. Optionally, filter entry-lines based on *where* function.

Dumping stats
-------------
The first Haka script (``stats_on_exit``) gives an usage of the above api.

.. literalinclude:: ../../../sample/stats/stats_on_exit.lua
    :tab-width: 4
    :language: lua

The script will output some statistics on collected http trafic after parsing all packets in the provided pcap file (i.e. at Haka exit). Below, a snippet output generated while running the Haka script file on the DARPA pcap file:

.. code-block:: console

    ...
    list of source ip using 'Mozilla/2.0' as user-gent'
    | ip             | useragent                 |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;) |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;) |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;) |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;) |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;) |
    ... 12762 remaining entries
    ...

Getting stats in interactive mode
---------------------------------
The second script (``stats_interactive``) fills the `stats` table with http info (thanks to the ``stats.lua`` script) and then launches the intercative mode after parsing all packets in the provided pcap file. Statistics are then available through the `stats` variable.

.. literalinclude:: ../../../sample/stats/stats_interactive.lua
    :tab-width: 4
    :language: lua

Hereafter, ``hakapcap`` output when entering the interactive mode:

.. code-block:: console

    entering interactive session: entering interactive mode for playing statistics
    Statistics are available through 'stats' variable. Run
        - stats:list() to get the list of column names
        - stats:top(column) to get the top 10 of selected field
        - stats:dump(nb) to dump 'nb' entries of stats table
        - stats:select_table({column_1, column_2, etc.}, cond_func)) to select some columns and filter them based on 'cond_func' function
    Examples:
        - stats:top('useragent')
        - stats:select_table({'resource', 'status'}, function(elem) return elem.status == '404' end):top('resource')
   >

.. note:: Press CTRL-D to leave the interactive mode

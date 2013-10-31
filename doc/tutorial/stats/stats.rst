
Statistics
==========

Introduction
------------
This tutorial shows how tu collect trafic and make statisctics on collected data.

How-to
------
This tutorial introduces two lua script files: ``stats_on_exit`` and ``stats_interactive`` which could be run using the ``hakapcap`` tool as follows:

.. parsed-literal::

    $ cd |haka_install_path|/share/haka/sample/tutorial/stats
    $ hakapcap <pcap_file> <script_stat_file>

.. note:: The tutorial provides in |haka_install_path|/share/haka/sample/tutorial/stats a pre-processed pcap file originated from the DARPA-99 dataset and which could be retrieved from http://www.ll.mit.edu/mission/communications/cyber/CSTcorpora/ideval/data/1999/training/week1/monday/outside.tcpdump.gz).

Collecting data
---------------
Before making statisctics, we need first to collect data. This is the purpose of ``stats.lua`` file which starts by creating a global `stats` table. This table will be updated with http info whenever a new http response is received. An entry in the `stats` table consists of the following fields:

- ip: source ip
- method: http request method (`get`, `post`, etc.)
- host: http host
- ressource: normalized path
- referer: referer header
- usergant: user-agent header
- status: response status code

.. note:: While the security rule 'hooks' on `http-response` we still get access to http request fields (via `request` accessor) and ip header fields (through `connection` accessor)

.. highlightlang:: lua

.. literalinclude:: ../../../sample/tutorial/stats/stats.lua
    :tab-width: 4

Stats API
---------
This section introduces the stats API. More precisely, it shows how to create the global `stats` table and how to run basic stats operations on the created table.

.. lua:module:: tblutils

.. lua:function:: new()

    Create the stats table

.. lua:module:: stats

.. lua:function:: list(self)

    Print column names of `stats` table

.. lua:function:: dump(self, nb)

    Print `nb` entries of `stats` table

.. lua:function:: top(self, column_name, nb)

    Dump the top 10 of given field name. Limits output to `nb` if `nb` is provided

.. lua:function:: select_table(self, column_tab, where)

    Select specific columns from `stats` table. Optionnaly, filter entry-lines based on `where` function

Dumping stats
-------------
The first lua script (``stats_on_exit``) gives an usage of the above api.

.. literalinclude:: ../../../sample/tutorial/stats/stats_on_exit.lua
    :tab-width: 4

The script will output some statistics on collected http trafic after parsing all packets in the provided pcap file (i.e. at haka exit). Below, a snippet output generated while running the lua script file on the DARPA pcap file:

.. parsed-literal::
    ...
    list of source ip using 'Mozilla/2.0' as user-gent'
    | ip             | useragent                                       |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;)                       |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;)                       |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;)                       |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;)                       |
    | 172.16.117.132 | Mozilla/2.01 (Win3.1; I;)                       |
    ... 12762 remaining entries
    ...

Getting stats in interactive mode
---------------------------------
The second script (``stats_interactive``) fills the `stats` table with http info (thanks to the ``stats.lua`` script) and then launches the intercative mode after parsing all packets in the provided pcap file. Statistics are then available through the `stats` variable.

.. literalinclude:: ../../../sample/tutorial/stats/stats_interactive.lua
    :tab-width: 4

Herefter, hakapcap output when entering the interactive mode:

.. parsed-literal::

    entering interactive session: entering interactive mode for playing statistics
    Statistics are available through 'stats' variable. Run
        - stats:list() to get the list of column names
        - stats:top(column) to get the top 10 of selected field
        - stats:select_dump(nb) to dump 'nb' entries of stats table
        - stats:select_table({column_1, column_2, etc.}, cond_func)) to select some columns and filter them based on 'cond_func' function
    Examples:
        - stats:top('useragent')
        - stats:select_table({'ressource', 'status'}, function(elem) return elem.status == '404' end):top('ressource')
   >

.. note:: Press CTRL-D to leave the interactive mode

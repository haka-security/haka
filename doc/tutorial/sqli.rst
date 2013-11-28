
SQLi attack detection
=====================

Introduction
------------
This tutorial shows how tu use Haka in order to detect SQL injection attacks (SQLi). Note that our goal is not to block 100% of SQLi attacks (with 0% false-positive rate) but to show how to build iteratively an sqli filtering policy thanks to haka capabilities. 

How-to
------
This tutorial introduces a set of lua script files located at <haka_install_path>/share/haka/sample/sqli and which could be run using the ``hakapcap`` tool:

.. code-block:: console

    $ cd <haka_install_path>/share/haka/sample/qli
    $ hakapcap sqli.pcap sqli-sample.lua

All the samples are self-documented.

Writing http rules
------------------
To write http rules, we need first to load the ipv4, tcp and http dissectors and set the next dissector to `http` when the tcp port is equal to 80 (this is done on connection establishment thanks to the `tcp-connection-new` hook). This is the purpose of the ``httpconfig.lua`` which is required by all the samples given in this tutorial.

.. highlightlang:: lua

.. literalinclude:: ../../sample/sqli/httpconfig.lua
    :tab-width: 4

My first naive rule
-------------------
The first example presents a naive rule which checks some malicious patterns against the whole uri. A score is updated whenever an sqli keywords is found (`select`, `update`, etc.). An alert is raised if the score exceeds a predefined threshold.

.. literalinclude:: ../../sample/sqli/sqli-simple.lua
    :tab-width: 4

Anti-evasion
------------
It is trivial to bypass the above rule with slight modifications on uri. For instance hiding a select keyword using comments (e.g. `sel/*something*/ect`) or simply using uppercase letters will bypass our naive rule. The script file ``sqli-decode.lua`` improves detection by applying first decoding fucntions on uri. This fucntions are defined in ``httpdecode.lua`` file.

.. literalinclude:: ../../sample/sqli/sqli-decode.lua
    :tab-width: 4

Fine-grained analysis
---------------------
All the above rules check the malicious patterns against the whole uri. The purpose of this scenario (``sqli-fine-grained.lua``) is to leverage the :lua:mod:`http` api in order to check the patterns against only subparts of the http request (query's argument, list of cookies).

.. literalinclude:: ../../sample/sqli/sqli-fine-grained.lua
    :tab-width: 4

Mutliple rules
--------------
The script file introduces additional malicious patterns and use the rule_group feature to define multiple anti-sqli security rules. Each rule focus on the detection of a particular pattern (sql keywords, sql comments, etc.)

.. literalinclude:: ../../sample/sqli/sqli-groups.lua
    :tab-width: 4

.. note:: Decoding functions are applied depending on the pattern. It is obvious to not apply uncomment function when we are looking for comments.

White list
----------
All the defined rules are too general and will therefore raise many alerts. In the example given hereafter we show how we could skip evalation of rules if the uri matches some conditions (for instance do not evaluate anti-sqli rules when the requested resource is equal to `/foo/bar/safepage.php`). This is shows another advantage of using rules group feature.

.. note:: The check is done after uri normalisation

.. literalinclude:: ../../sample/sqli/sqli-white-list.lua
    :tab-width: 4

Going further
-------------
As mentioned in the top of this tutorial, our aim is not to block all SQLi attacks. To improve detection rate, one could extend the malicious patterns given throughout these examples and make use of a powerful regular expression engine.

Haka v0.4.0
===========

New features
------------

  * Support for Kibana 4
  * New policy concept to ease basic configuration by removing the need to
    write rule in this case
  * Dissector chaining improvements, the dissector to be applied next is now
    decided though the policy `next_dissector`
  * New dissector `parent()` function
  * New dissector automatic field delegation to access field located on a
    parent dissector
  * Various bug fixes

Breaking changes
----------------

  * Protocol modules no longer export their dissector, instead they are
    registered in the object `haka.dissectors`. This one need to be used to
    access any event, policy or helper function.

Haka v0.3.0
===========

New features
------------

  * Stream-based asm instruction disassembler module based on Capstone engine
  * Logging performance improvements
  * Various bug fixes

Haka v0.2.2
===========

New features
------------

 * Add support for 802.1q
 * Various bug fixes
   * Fix Elasticsearch module thread concurrency
   * Fix cross compilation
   * Fix Lua stack leak

Haka v0.2.1
===========

New features
------------

  * Support the release of Hakabana
  * Multiple modules added
    * GeoIP
    * Elasticsearch database connector
    * Elasticsearch alert module
  * Various bug fixes


Haka v0.2
=========

New features
------------

  * Protocol dissector written in Lua
    * Protocol message syntax described in Lua
    * Protocol state machine described in Lua
  * Multiple dissectors added
    * ICMP
    * UDP
    * DNS
  * Pattern matching engine
  * External module support
  * IPv4 fragmentation support
  * Remote console

Haka v0.1.0
===========

First release of Haka.

New features
------------

  * Security rules written in Lua
  * Logging and alerting
  * Pcap packet capture
  * Netfilter queue packet capture
  * Security rules debugging
  * Multiple dissectors
    * IPv4
    * TCP
    * HTTP

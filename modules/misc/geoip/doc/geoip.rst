.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Geoip
=====

.. haka:module:: geoip

Geoip utility module.

**Usage:**

::

    local geoip = require('misc/geoip')

API
---

.. haka:function:: country(ip) -> country_code

    :param ip: IPv4 address.
    :ptype ip: :haka:class:`ipv4.addr`
    :return country_code: Result country code or nil if not found.
    :rtype country_code: string

    Query the country code for an IP address.

Example
-------

::

    local geoip = require('misc/geoip')
    local ipv4 = require('protocol/ipv4')

    print(geoip.country(ipv4.addr("8.8.8.8"))


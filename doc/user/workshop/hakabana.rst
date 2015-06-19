.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Hakabana
========

Hakabana is a monitoring tool that uses Kibana and Elasticsearch to visualize
traffic passing through Haka in *real-time*. The package is already installed
in the haka-live iso but can be downloaded directly from http://www.haka-security.org.

Getting started
^^^^^^^^^^^^^^^

Hakabana module is installed at ``/usr/share/haka/modules/misc/hakabana``.
It consists of a set of security rules that export network traffic to
Elasticsearch server. They are then displayed thanks to our Kibana dashboard.

Hakabana ships with a default configuration allowing starting quickly with
traffic monitoring. It is available in ``/usr/share/haka/hakabana``

.. admonition:: Exercise

    * follow the instruction below to start haka:

    .. code-block:: console

        cd /usr/share/haka/hakabana
        sudo haka -c haka.conf

    * visit the url: http://localhost/kibana/ and load hakabana dashboard
      from ``/usr/share/haka/hakabana/dashboard/``

I want more DNS info
^^^^^^^^^^^^^^^^^^^^

Your goal here is to customize the security rules in order to export extra data.

.. admonition:: Exercise

    * update the ``dns.lua`` in order to export dns types.

    * add a panel to hakabana dashboard to display dns types.


Geo localization
^^^^^^^^^^^^^^^^

.. admonition:: Optional

    Hakabana features a `geoip` module allowing to get the country code associated to an ip
    address. Here is an example using it:

    .. code-block:: lua

        local ipv4 = require('protocol/ipv4')

        local geoip_module = require('misc/geoip')
        local geoip = geoip_module.open('/usr/share/GeoIP/GeoIP.dat')

        haka.rule {
            hook = ipv4.events.receive_packet,
            eval = function (pkt)
                local dst = pkt.dst
                haka.log("ip %s from %s",dst, geoip:country(dst))
            end
        }

    .. admonition:: Exercise

        * update the ``flow.lua`` file in order to exclude traffic addressed to a given
          country.


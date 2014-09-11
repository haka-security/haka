.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Elasticsearch alert `alert/json`
================================

Description
^^^^^^^^^^^

This module will exports all alerts to an elasticsearch server. The module adds also
some extra information such as geoip data.

Parameters
^^^^^^^^^^

.. describe:: elasticsearch

    Elasticsearch server address.

.. describe:: geoip

    Absolute file path to geoip data file.

Example :

.. code-block:: ini

    elasticsearch = "http://127.0.0.1:9200"
    geoip = "/usr/share/GeoIP/GeoIP.dat"


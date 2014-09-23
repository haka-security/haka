.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Elasticsearch alert `alert/elasticsearch`
=========================================

Description
^^^^^^^^^^^

This module will exports all alerts to an elasticsearch server. It also adds also
some extra information such as geoip data.

Parameters
^^^^^^^^^^

.. describe:: elasticsearch_server

    Elasticsearch server address.
 
    .. warning:: Be careful not to create security rules that block elasticsearch traffic.


.. describe:: elasticsearch_index

    Elasticsearch index.

    .. note:: If this field is missing, Haka will use ``ips`` as default kibana index.

.. describe:: geoip_database

    Absolute file path to geoip data file. Optional field that provides
    geolocalization support.

Example :

.. code-block:: ini

    [alert]
    # Select the alert module
    module = "alert/elasticsearch"

    # alert/elasticsearch module option
    elasticsearch_server = "http://127.0.0.1:9200"
    #elasticsearch_index = "ips"
    geoip_database = "/usr/share/GeoIP/GeoIP.dat"

Kibana dashboard
^^^^^^^^^^^^^^^^

The dashboard :download:`ips_dahsboard.json` is an example of a Kibana dashboard that shows some info about haka alerts.

.. note:: Set the elasticsearch index to ``elasticsearch_index`` value in the main kibana dashboard setting.

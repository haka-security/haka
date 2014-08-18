.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

ElasticSearch
=============

.. haka:module:: elasticsearch

ElasticSearch database connector for Haka.

**Usage:**

::

    local elasticsearch = require('misc/elasticsearch')

API
---

.. haka:function:: connector(host) -> connector

    :param host: ElasticSearch host name.
    :ptype host: string

    Create a new ElasticSearch connector and connect to the given address.

.. haka:class:: Connector

    .. haka:method:: Connector:newindex(index, data)

        :param index: ElasticSearch index name.
        :ptype index: string
        :param data: Data to pass to the ElasticSearch server (check the
            ElasticSearch API for more detail about it).
        :ptype data: table

        Create a new index in the ElasticSearch database.

    .. haka:method:: Connector:insert(index, type, id, data)

        :param index: ElasticSearch index name.
        :ptype index: string
        :param type: Objec type.
        :ptype type: string
        :param id: Optional object id (can be ``nil``).
        :ptype id: string
        :param data: Object data.
        :ptype data: table

        Insert a new object in the ElasticSearch database.

    .. haka:method:: Connector:update(index, type, id, data)

        :param index: ElasticSearch index name.
        :ptype index: string
        :param type: Objec type.
        :ptype type: string
        :param id: Object id.
        :ptype id: string
        :param data: Object data to update.
        :ptype data: table

        Update some data of an existing object in the ElasticSearch database.

    .. haka:method:: Connector:timestamp(time) -> formated

        :param time: Time.
        :ptype time: :haka:class:`haka.time`
        :return formated: Formated timestamp in ElasticSearch format.
        :rtype formated: string

        Render a timestamp to the standard ElasticSearch format.

    .. haka:method:: Connector:genid() -> id

        :return id: Unique id.
        :rtype id: string

        Generate a unique id which can be used as an object id.

        .. seealso:: :haka:func:`<Connector>.insert`.

Example
-------

::

    local elasticsearch = require('misc/elasticsearch')

    local connector = elasticsearch.connector('http://127.0.0.1:9200')

    connector:insert("myindex", "mytype", nil, { name="object name" })


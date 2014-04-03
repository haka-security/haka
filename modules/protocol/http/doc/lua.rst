.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

HTTP
====

Utilities
---------

.. lua:module:: http.uri

.. lua:class:: HttpUriSplit

    .. lua:data:: scheme

        URI scheme.

    .. lua:data:: authority

        URI authority.

    .. lua:data:: host

        URI host.

    .. lua:data:: userinfo

        URI user information.

    .. lua:data:: user

        URI user (from `userinfo`).

    .. lua:data:: pass

        URI password (from `userinfo`).

    .. note:: rfc 3986 states that the format "user:password" in the `userinfo` field is deprecated.

    .. lua:data:: port

        URI port.

    .. lua:data:: path

        URI path.

    .. lua:data:: query

        URI query.

    .. lua:data:: args

        URI query's parameters.

    .. lua:data:: fragment

        URI fragment.

    .. lua:function:: tostring(split_uri)

        Recreate the URI.

    .. lua:method:: normalize()

        Normalize URI according to rfc 3986: remove dot-segments in path, capitalize letters in esape sequences,
        decode percent-encoded octets (safe decoding), remove default port, etc.

.. lua:function:: split(str)

    Split URI into subparts.

    Example: ::

        http.uri.split('http://www.example.com:8888/foo/page.php')

.. lua:function:: normalize(str)

    Normalize URI according to rfc 3986.

    .. seealso:: :lua:func:`split:normalize`


.. lua:module:: http.cookies

.. lua:class:: HttpCookiesSplit

    Store the cookies as a table of key-value pairs.

    .. lua function tostring(cookies)

        Return cookies as a string.

.. lua:function:: cookies(str)

    Parse the cookies.

    :returns: :lua:class:`cookies`


Dissector
---------

.. lua:module:: http

The HTTP dissector supports:

* HTTP 1.0 and 1.1
* Chunked transfer mode
* Request analysis
* Reponse analysis
* Keep-alive connections

Events
^^^^^^

* `request`

    Triggered when the dissector recognizes a valid HTTP request. The request can be modified
    in this event.

* `request-data`

    Whenever the dissector receives some HTTP data associated with a request, this event is trigger.
    The data are represented by a stream.

* `response`

    Triggered when the dissector recognizes a valid HTTP response. The response can be modified
    in this event. It can also read the request information.

* `response-data`

    Whenever the dissector receives some HTTP data associated with a response, this event is trigger.
    The data are represented by a stream.


Functions
^^^^^^^^^

.. lua:class:: HttpRequestResult

    Hold information about the current request. It is accessible from any event.

    .. lua:data:: method
                  uri
                  version

        Request line elements.

    .. lua:data:: headers

        Headers table.

.. lua:class:: HttpResponseResult

    Hold information about the current response. It is accessible only in the event
    associated with a response.

    .. lua:data:: version
                  status
                  reason

        Response line elements.

    .. lua:data:: headers

        Headers table.

.. lua:class:: http

    .. lua:data:: request
                  response

        Access to parsed request and response.

    .. lua:method:: drop()

        Drop the current HTTP connection.

    .. lua:method:: reset()

        Reset the current HTTP connection. The client and the server will be notified
        with RST tcp packets.

    .. lua:method:: enable_data_modification()

        Activate the data transformation to allow modification to be done on the HTTP data.

.. lua:function:: install_tcp_rule(port)

    Install a rule to enable the HTTP dissector on the given tcp `port`.

Example
-------

.. literalinclude:: ../../../../sample/ruleset/http/policy.lua
    :language: lua
    :tab-width: 4

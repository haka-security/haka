
.. highlightlang:: lua

HTTP
====

.. lua:module:: http.uri

Types
-----

.. lua:class:: split

    .. lua:data:: scheme

        URI scheme.

    .. lua:data:: authority

        URI authority.

    .. lua:data:: host

        URI host.

    .. lua:data:: userinfo

        URI userinfo.

    .. lua:data:: user

        URI user (from userinfo).

    .. lua:data:: pass

        URI password (from userinfo).

    .. note:: rfc 3986 states that the format "user:password" in the userinfo field is deprecated.

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


.. lua:class:: cookies

    Store the cookies as a table of key-value pairs.

    .. lua function tostring(cookies)

        Return cookies as a string.

.. lua:function:: cookies(str)

    Parse the cookies.

    :returns: :lua:class:`cookies`



Functions
---------

.. lua:function:: normalize(str)

    Normalize URI according to rfc 3986.

    .. seealso:: :lua:func:`split:normalize`




Dissector
---------

.. lua:module:: http

.. lua:class:: http

    Dissector data for HTTP. In addition to the usuall `http-up` and  `http-down`, HTTP register two additional
    hooks:

    * `http-request`: called when a request is fully parsed.
    * `http-response`: called when a response is fully parsed.

    .. seealso:: :lua:class:`haka.dissector_data`.

    .. lua:data:: request

        Inside a `http-request` or `http-response` hook, the :lua:data:`http:request` table holds information about the
        current request.

        .. lua:data:: method
                      uri
                      version

            Request line elements.

        .. lua:data:: headers

            Headers table.

        .. lua:data:: data

            Stream of HTTP data.

    .. lua:data:: response

        Inside a `http-response` hook, the :lua:data:`http:response` table holds information about the current response.

        .. note:: This table is not available inside the hook `http-request`.

        .. lua:data:: version
                      status
                      reason

            Response line elements.

        .. lua:data:: headers

            Headers table.

        .. lua:data:: data

            Stream of HTTP data.

Example
-------

.. literalinclude:: ../../../../sample/ruleset/http/policy.lua
    :language: lua
    :tab-width: 4

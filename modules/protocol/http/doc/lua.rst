
.. highlightlang:: lua

HTTP
====

.. lua:module:: http

Types
-----

.. lua:class:: uri.split

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

    .. note:: rfc 3986 states that the format "user:password" in the userinfo field is deprecated

    .. lua:data:: port

        URI port.

    .. lua:data:: path

        URI path.

    .. lua:data:: query

        URI query.

    .. lua:class :: args

        URI query's parameters.

    .. lua:data:: fragment

        URI fragment.

    .. lua:function:: tostring(uri.split)

		unsplit URI.

    .. lua:function:: normalize(uri.split)

		Normalize URI according to rfc 3986

.. lua:function:: uri.split(str)

    Split URI into subparts.

    Example: ::

	    uri.split('http://www.example.com:8888/foo/page.php')

.. lua:class:: uri.cookies

    .. lua function tostring(uri.cookies)

		 return cookie string

.. lua:class:: uri.cookies(str)

	Store cookies as a list of key-value pairs.

Functions
---------

.. lua:function:: uri.normalize(str)

	Normalize URI according to rfc 3986: remove dot-segments in path, capitalize letters in esape sequences, decode percent-encoded octets (safe decoding), remove default port, etc.




Dissector
---------

.. lua:class:: http

    Dissector data for HTTP. In addition to the usuall `http-up` and  `http-down`, HTTP register two additional
    hooks:

    * `http-request`: called when a request is fully parsed.
    * `http-response`: called when a response is fully parsed.

    .. seealso:: :lua:class:`haka.dissector_data`.

    .. lua:data:: request

        .. lua:data:: method
                      uri
                      version

            Request line elements.

        .. lua:data:: headers

            Headers table.

        .. lua:data:: data

            Stream of HTTP data.

    .. lua:data:: response

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

.. literalinclude:: ../../../../sample/ruleset/proto/http/policy.lua
    :language: lua
    :tab-width: 4

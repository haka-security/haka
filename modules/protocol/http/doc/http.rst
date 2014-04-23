.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Http
====

.. haka:module:: http

.. haka:class:: HttpDissector
    :module:
    :objtype: dissector

    :Name: ``'http'``
    :Extend: :haka:class:`haka.dissector.FlowDissector` |nbsp|
    
    HTTP protocol dissector supporting the following features:
    
    * HTTP 1.0 and 1.1
    * Chunked transfer mode
    * Request analysis
    * Reponse analysis
    * Keep-alive connections
    
    .. haka:function:: install_tcp_rule(port)
    
        :param port: TCP port number.
        :paramtype port: number
        
        Install a rule on TCP to enable HTTP dissection on the given port number.
    
    .. haka:function:: dissect(flow)
    
        :param flow: Upper flow dissector.
        :paramtype flow: :haka:class:`haka.dissector.FlowDissector`
        
        Activate HTTP dissection on the given flow.
    
    .. haka:attribute:: HttpDissector.flow
    
        Flow from which HTTP is built.
        
    .. haka:method:: HttpDissector:enable_data_modification()
    
        Activate data modification for all data received along with the current request
        or response. This mode allow to modify on the fly any data without having to handle
        chunk sizes or the content-length header.
        
    .. haka:method:: HttpDissector:drop()
    
        Drop the associated flow.
        
    .. haka:method:: HttpDissector:reset()

        Reset the current HTTP connection. The client and the server will be notified
        with RST tcp packets.

Protocol elements
-----------------

.. haka:class:: HttpHeaders

    Parsed headers information for HTTP.
    
    .. haka:operator:: HttpHeaders[name] -> value
                       HttpHeaders[name] = newvalue

        :param name: Header name.
        :paramtype name: string
        :param newvalue: New header value.
        :paramtype newvalue: string
        :return value: Current header value.
        :rtype value: string

        Access and modify any HTTP header.
        
    .. haka:operator:: pairs(HttpHeaders) -> key,value
    
        :return key: Header key.
        :rtype key: string
        :return value: Header value.
        :rtype value: string
        
        Return an iterator over all headers.

.. haka:class:: HttpRequest

    Parsed information about a request.

    .. haka:attribute:: HttpRequest:method
                        HttpRequest:uri
                        HttpRequest:version
                        
        :type: string

        Request line elements.

    .. haka:attribute:: HttpRequest:headers
    
        :type: :haka:class:`HttpHeaders` |nbsp|

        Headers table.

.. haka:class:: HttpResponse

    Parsed information about a response.

    .. haka:attribute:: HttpRequest:version
                        HttpRequest:status
                        HttpRequest:reason
                        
        :type: string

        Response line elements.

    .. haka:attribute:: HttpRequest:headers
    
        :type: :haka:class:`HttpHeaders` |nbsp|

        Headers table.

Events
------

.. haka:function:: http.events.request(http, request)
    :module:
    :objtype: event
    
    :param http: HTTP dissector.
    :paramtype http: :haka:class:`HttpDissector`
    :param request: HTTP request data.
    :paramtype request: :haka:class:`HttpRequest`
    
    Event triggered whenever a new HTTP request is received.

.. haka:function:: http.events.response(http, response)
    :module:
    :objtype: event
    
    :param http: HTTP dissector.
    :paramtype http: :haka:class:`HttpDissector`
    :param response: HTTP response data.
    :paramtype response: :haka:class:`HttpResponse`
    
    Event triggered whenever a new HTTP response is received.

.. haka:function:: http.events.receive_data(http, stream, current)
    :module:
    :objtype: event
    
    :param http: HTTP dissector.
    :paramtype http: :haka:class:`HttpDissector`
    :param stream: TCP data stream.
    :paramtype stream: :haka:class:`vbuffer_stream`
    :param current: Current position in the stream.
    :paramtype current: :haka:class:`vbuffer_iterator`
    :param direction: Data direction (``'up'`` or ``'down'``).
    :paramtype direction: string
    
    Event triggered when some data are available on either a request or a response.
    
    **Event options:**
    
        .. haka:data:: streamed
            :noindex:
            :module:
            
            :type: boolean
            
            Run the event listener as a streamed function.

.. haka:function:: http.events.request_data(http, stream, current)
    :module:
    :objtype: event
    
    :param http: HTTP dissector.
    :paramtype http: :haka:class:`HttpDissector`
    :param stream: TCP data stream.
    :paramtype stream: :haka:class:`vbuffer_stream`
    :param current: Current position in the stream.
    :paramtype current: :haka:class:`vbuffer_iterator`
    
    Event triggered when some data are available on a request.
    
    **Event options:**
    
        .. haka:data:: streamed
            :noindex:
            :module:
            
            :type: boolean
            
            Run the event listener as a streamed function.

.. haka:function:: http.events.response(http, stream, current)
    :module:
    :objtype: event
    
    :param http: HTTP dissector.
    :paramtype http: :haka:class:`HttpDissector`
    :param stream: TCP data stream.
    :paramtype stream: :haka:class:`vbuffer_stream`
    :param current: Current position in the stream.
    :paramtype current: :haka:class:`vbuffer_iterator`
    
    Event triggered when some data are available on a response.
    
    **Event options:**
    
        .. haka:data:: streamed
            :noindex:
            :module:
            
            :type: boolean
            
            Run the event listener as a streamed function.


Utilities
---------

.. haka:module:: http.uri

.. haka:class:: HttpUriSplit
    :module:

    .. haka:function:: split(uri) -> split
    
        :param uri: Uri to split.
        :paramtype uri: string
        :return split: New split uri representation.
        :rtype split: :haka:class:`HttpUriSplit`
        
        Split uri into subparts.

        **Example:** ::
    
            http.uri.split('http://www.example.com:8888/foo/page.php')
    
    .. haka:attribute:: HttpUriSplit:scheme
    
        :type: string

        Uri scheme.

    .. haka:attribute:: HttpUriSplit:authority
    
        :type: string

        Uri authority.

    .. haka:attribute:: HttpUriSplit:host
    
        :type: string

        Uri host.

    .. haka:attribute:: HttpUriSplit:userinfo
    
        :type: string

        Uri user information.

    .. haka:attribute:: HttpUriSplit:user
    
        :type: string

        Uri user (from *userinfo*).

    .. haka:attribute:: HttpUriSplit:pass
    
        :type: string

        Uri password (from *userinfo*).

        .. note:: rfc 3986 states that the format "user:password" in the `userinfo` field is deprecated.

    .. haka:attribute:: HttpUriSplit:port
    
        :type: string

        Uri port.

    .. haka:attribute:: HttpUriSplit:path
    
        :type: string

        Uri path.

    .. haka:attribute:: HttpUriSplit:query
    
        :type: string

        Uri query.

    .. haka:attribute:: HttpUriSplit:args
    
        :type: table

        Uri query's parameters.

    .. haka:attribute:: HttpUriSplit:fragment
    
        :type: string

        Uri fragment.

    .. haka:operator:: tostring(HttpUriSplit) -> str

        :return str: Full uri.
        :rtype str: string

        Recreate the uri full string.

    .. haka:method:: HttpUriSplit:normalize()

        Normalize URI according to rfc 3986: remove dot-segments in path, capitalize letters in esape sequences,
        decode percent-encoded octets (safe decoding), remove default port, etc.

.. haka:function:: normalize(uri) -> norm_uri

    :param uri: Uri.
    :paramtype uri: string
    :return norm_uri: Normalized uri.
    :rtype norm_uri: string

    Normalize URI according to rfc 3986.

    .. seealso:: :haka:func:`<HttpUriSplit>.normalize()`


.. haka:module:: http.cookies

.. haka:class:: HttpCookiesSplit

    Store the cookies as a table of key-value pairs.

    .. haka:function:: split(cookies) -> split_cookies
    
        :param cookies: Cookies line extracted from the HTTP headers.
        :paramtype cookies: string
        :return split_cookies: Split cookies.
        :rtype split_cookies: :haka:class:`HttpCookiesSplit`
    
        Parse the cookies and split them.

    .. haka:operator:: tostring(HttpCookiesSplit) -> str
    
        :return str: String containing all cookies.
        :rtype str: string

        Return all cookies as a string.
        
    .. haka:operator:: HttpCookiesSplit[key] -> value
                       HttpCookiesSplit[key] = newvalue

        :param name: Cookie name.
        :paramtype name: string
        :param newvalue: New cookie value.
        :paramtype newvalue: string
        :return value: Current cookie value.
        :rtype value: string

        Access and modify any cookies in the table.

Example
-------

.. literalinclude:: ../../../../sample/ruleset/http/policy.lua
    :language: lua
    :tab-width: 4

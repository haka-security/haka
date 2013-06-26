
.. highlightlang:: lua

HTTP
====

.. lua:module:: http


Dissector
---------

.. lua:class:: http

    Dissector data for HTTP.

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


Hooks
^^^^^

In addition to the usuall `http-up` and  `http-down`, HTTP register two additional
hooks:

* `http-request`
* `http-response`

Example
-------

.. code-block:: lua

    haka.rule {
        hooks = {"http-request"},
        eval = function (self, http)
            local method = http.request.method:lower()
            if method ~= 'get' and method ~= 'post' then
                haka.log.warning("filter", "forbidden http method '%s'", method)
            end
        end
    }

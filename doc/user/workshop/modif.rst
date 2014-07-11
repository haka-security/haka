
On-the-fly modifications
========================

Haka can also be used to alter packet content as they pass through the system.
The following example will try to blur the web. More specifically, it will add
some css trick on every html pages.

HTTP dissector
--------------

As usual we first need to load the http dissector named ``protocol/http``.

As there is no mechanism over TCP to specify the next protocol (unlike IP) we
also have to tell Haka which TCP connection it will have to dissect with the
HTTP dissector.

.. code-block:: lua

    http.install_tcp_rule(80)

.. admonition:: Exercise

    By going to the documentation of the HTTP dissector we can see that it provides
    an interesting event called ``http.events.request``. This event will pass the
    HTTP connection and the HTTP request to the eval function.

    Create a rule hooked on ``http.events.request``. Check it work correctly by
    logging some part of the request object. You can use haka on real traffic or
    download the following pcap file: :download:`http.pcap`.

Modifying http request
----------------------

From this request object we are able to get the HTTP headers. It is located in the
request field available in the dissector object, for instance:

.. code-block:: lua

    request.headers['Host']

We will first create a new rule on the request to make sure that the response data
will be transmitted in clear text. This is required as we want to modify the HTTP
page.

.. admonition:: Exercise

    In order to be sure to receive a clear text response (not gzipped) it is
    required to change the following headers:

        * remove ``Accept-Encoding``
        * set ``Accept`` to ``'*/*'``

    Write a rule hooked on ``http.events.request`` that do the required change.

.. admonition:: Exercise

    Also to avoid cache issues, it is easy to remove some cache control headers:

        * remove ``If-Modified-Since``
        * and remove ``If-None-Match``

    Update your rule.

Modifying http response
-----------------------

Request
^^^^^^^

Our goal is to insert data inside the HTTP page, to be able to do it correctly,
a rule on the event ``http.events.response`` need to be added. It must call the
function :haka:func:`<HttpDissector>.enable_data_modification()`. This will enable
our next rule to insert its data without having to take care of updating the
*Content-Length* header value for instance.

CSS
^^^

To blur the web, we will need to add the following snippet:

.. code-block:: css

    * {
        color: transparent !important;
        text-shadow: 0 0 3px black !important;
    }

We can define a simple variable to save it:

.. code-block:: lua

    local css = '<style type="text/css" media="screen"> * { color: transparent !important; text-shadow: 0 0 3px black !important; } </style>'


Stream
^^^^^^

In order to add the css snippet on every html page we can use the
``http.events.response_data`` event of the HTTP dissector.

This event will be triggered each time response data will be available. This
event have a very interesting and powerful option, namely ``streamed``.

.. code-block:: lua

    haka.rule{
        hook = http.events.response_data,
        options = {
            streamed = true,
        },
        eval = function (flow, iter)
            -- Eval function
        end
    }

This option turn on the streamed mode. Consequently the eval function will be
called once, and only once. It will automatically wait for data when needed
if it uses `stream functions`.

Next step is to look for the right place in the stream to insert our css
snippet. Namely in the ``<head></head>`` section.  In order to find where is
this section in the response data we can use a regular expression.

Regular expression
^^^^^^^^^^^^^^^^^^

Regular expression are available through modules in Haka. Currently Haka provide
only one regular expression module : ``pcre``.

.. code-block:: lua

    local rem = require('regexp/pcre')

Then we have to compile a new regexp.

.. code-block:: lua

    local regexp = rem.re:compile("</head>", rem.re.CASE_INSENSITIVE)

It is important to note that regexp module is stream aware, and so, it can read
an entire stream looking for a given pattern before returning.

vbuffer iterator
^^^^^^^^^^^^^^^^

Both the eval function of the ``http.events.response_data`` event and the
``match()`` function of the regexp module use a special object called an
`iterator`. This iterator represent a point on the data stream. It can advance
on the stream and it can be used to manipulate (insert, remove, replace) it.

For example, finding the right place to insert our css snippet is as simple as:

.. code-block:: lua

    local result = regexp:match(iter, true)

``result`` will be a sub-buffer representing the matching part of our regexp in
the data stream.

Finally if we want to insert our snippet we can use the vbuffer API:

.. code-block:: lua

    result:pos('begin'):insert(haka.vbuffer_from(css))

.. admonition:: Exercise

    Bring all the pieces together in one rule and test it on live traffic.

    Test it on http traffic not on https. http://www.haka-security.org might be
    a good candidate for it.

Full script
-----------

You will find the full script here :download:`blurring-the-web.lua`.


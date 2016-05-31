.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

    By going to the documentation of the HTTP dissector we can see that it
    provides an interesting event called ``http.events.request``. This event
    will pass the HTTP connection and the HTTP request to the eval function.

    Create a rule attached on ``http.events.request``. Check it work correctly
    by logging some part of the request object. You can use haka on real
    traffic or download the following pcap file: :download:`http.pcap`.

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

    Write a rule attached on ``http.events.request`` that do the required
    change.

.. admonition:: Exercise

    Also to avoid cache issues, it is easy to remove some cache control
    headers:

        * remove ``If-Modified-Since``
        * and remove ``If-None-Match``

    Update your rule.

Modifying http response
-----------------------

Our goal is to inject data inside a HTML page.

.. admonition:: Exercise

    The script :download:`blurring-the-web-attempt.lua` enables data
    modification on http responses and inject a CSS snippet code that blurs HTML
    web pages. Run the script and test it by visiting http://localhost/

.. note::

    The streamed option turns on the streamed mode. Consequently the eval function will be
    called once, and only once. It will automatically wait for data when needed
    if it uses `stream functions`.

As may have noticed, the script has no effect on page
http://localhost/blur.html. The next step is to look for the right place in the
stream to insert our css snippet. Namely in the ``<head></head>`` section. In
order to find where is this section in the response, data we can use a regular
expression.

The following code snippet loads the ``pcre`` module, then compiles a pattern: 

.. code-block:: lua

    local rem = require('regexp/pcre')
    local regexp = rem.re:compile("<head>", rem.re.CASE_INSENSITIVE)

Then, we rely on ``match`` function to check if data stream pointed to by an
iterator ``iter`` matches a pattern. If there is a match, the position of
``iter`` is updated and points right after the matching part in the data stream.
 
.. code-block:: lua

    local result = regexp:match(iter):

.. admonition:: Exercise

    Modify the script by using the regexp module in order to inject the css code
    at the end of the <head> section. Test it on http traffic not on https.
    http://www.haka-security.org might be a good candidate for it.

Full script
-----------

You will find the full script here :download:`blurring-the-web.lua`.

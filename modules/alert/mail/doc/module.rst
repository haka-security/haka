.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Mail alert `alert/mail`
=======================

Description
^^^^^^^^^^^

This module will exports all alerts and send them as email.

Parameters
^^^^^^^^^^

.. describe:: smtp_server

    SMTP server address. [Mandatory]

    .. warning:: Be careful not to create security rules that block SMTP traffic.

.. describe:: username

    SMTP account username, if authentication is used.

.. describe:: password

    SMTP account password, if authentication is used.

.. describe:: recipients

    Recipients of alert email. [Mandatory]

.. describe:: authentication

    Authentication method: SSL/TLS or STARTTLS.
    .. note:: Connection to SMTP server is done over ``SSL/TLS`` (port 465), by default.

.. describe:: alert_level

    Minimum level of alert to be sent : LOW, MEDIUM or HIGH.


Example :

.. code-block:: ini

    [alert]
    # Select the alert module
    module = "alert/mail"

    # alert/mail module option - Default SSL authentication
    smtp_server = "smtp.gmail.com"
    username = "admin@gmail.com"
    password = "my_secret"
    recipients = "elliot.alderson@allsafe.com"

    # alert/mail module option - No authentication
    smtp_server = "smtp.gmail.com"
    recipients = "admin@mycompany.com,another_admin@gmail.com"

    # alert/mail module option - Use STARTTLS
    smtp_server = "smtp.gmail.com:587"
    username = "admin@gmail.com"
    password = "my_secret"
    authentication = "STARTTLS"
    recipients = "admin@mycompany.com,another_admin@gmail.com"

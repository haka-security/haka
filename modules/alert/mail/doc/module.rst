.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Mail alert `alert/mail`
=========================================

Description
^^^^^^^^^^^

This module will exports all alerts and send them as email.

Parameters
^^^^^^^^^^

.. describe:: smtp_server

    SMTP server address.

    .. warning:: Be careful not to create security rules that block SMTP traffic.
    .. note:: Connection to SMTP server is done over ``SSL/TLS`` (port 465).

.. describe:: mail_username

    SMTP account username.

.. describe:: mail_password

    SMTP account password.

.. describe:: mail_recipient

    Recipients of alert email.

.. describe:: alert_level

    Minimum level of alert to be sent (Optional). [LOW, MEDIUM, HIGH]


Example :

.. code-block:: ini

    [alert]
    # Select the alert module
    module = "alert/mail"

    # alert/mail module option
    mail_server = "smtp.gmail.com"
    mail_username = "admin@gmail.com"
    mail_password = "my_secret"
    mail_recipient = "admin@mycompany.com,another_admin@gmail.com"

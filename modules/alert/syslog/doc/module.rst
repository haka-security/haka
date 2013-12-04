.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

Syslog alert `alert/syslog`
===========================

Description
^^^^^^^^^^^

Module that output alerts to syslog.

By default, the message are send to the facility local0. You can adjust your
syslogd.conf accordingly.

For instance: ::

    # HAKA syslog en Facility log level local0
    local0.*   /var/log/haka.log

Parameters
^^^^^^^^^^

This module does not have any parameters.

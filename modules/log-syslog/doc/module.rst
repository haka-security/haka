
.. program:: log-syslog

Syslog logging :program:`log-syslog`
====================================

Module that output log messages using syslog.

By default, the message are send to the facility local0. You can adjust your
syslogd.conf accordingly.

For instance: ::

    # HAKA syslog en Facility log level local0
    local0.*   /var/log/haka

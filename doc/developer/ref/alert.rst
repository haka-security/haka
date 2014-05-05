.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: c

Alert
=====

.. doxygenfile:: include/haka/alert.h

Example
-------

::

    ALERT(invalid_packet, 1, 1)
        description: L"invalid tcp packet, size is too small",
        severity: HAKA_ALERT_LOW,
    ENDALERT
    
    ALERT_NODE(invalid_packet, sources, 0, HAKA_ALERT_NODE_ADDRESS, "127.0.0.1");
    ALERT_NODE(invalid_packet, targets, 0, HAKA_ALERT_NODE_ADDRESS, "127.0.0.1");
    
    alert(invalid_packet);

.. This Source Code Form is subject to the terms of the Mozilla Public
.. License, v. 2.0. If a copy of the MPL was not distributed with this
.. file, You can obtain one at http://mozilla.org/MPL/2.0/.

.. highlightlang:: lua

Raw
===

.. haka:module:: raw

Raw dissector module.

**Usage:**

::

    local raw = require('protocol/raw')

Dissector
---------

.. haka:class:: RawDissector
    :module:
    :objtype: dissector

    :Name: ``'raw'``
    :Extend: :haka:class:`haka.helper.PacketDissector` and :haka:class:`packet`

    Raw packet dissector that is used as the first dissector for any received packet.

    .. haka:function:: create(size = 0) -> raw

        :param size: Size of the new packet.
        :paramtype size: number
        :return raw: Raw dissector.
        :rtype raw: :haka:class:`RawDissector`

        Create a new raw packet.

Options
-------

.. haka:data:: raw.options.drop_unknown_dissector

    :type: boolean
    :Default: ``false``

    If ``true``, any received packet that does not have a registered dissector will
    be dropped.

Events
------

.. haka:function:: raw.events.receive_packet(pkt)
    :module:
    :objtype: event

    :param pkt: Raw packet.
    :paramtype pkt: :haka:class:`RawDissector`

    Event that is triggered whenever a new raw packet is received.

.. haka:function:: raw.events.send_packet(pkt)
    :module:
    :objtype: event

    :param pkt: Raw packet.
    :paramtype pkt: :haka:class:`RawDissector`

    Event that is triggered just before sending a raw packet on the network.

Example
-------

::

    local pkt = raw.create(150)
    print(#pkt.payload)
    pkt:send()

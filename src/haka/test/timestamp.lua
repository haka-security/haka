-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

haka.rule {
	on = haka.dissectors.raw.events.receive_packet,
	eval = function (pkt)
		haka.log("packet timestamp is %ds %dns", pkt.timestamp.secs, pkt.timestamp.nsecs)
		haka.log("packet timestamp is %s seconds", pkt.timestamp.seconds)
		haka.log("packet timestamp is %s", pkt.timestamp)
	end
}

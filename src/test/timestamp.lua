-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('protocol/ipv4')

haka.rule {
	hooks = { 'ipv4-up' },
	eval = function (self, pkt)
		haka.log("timestamp", "packet timestamp is %ds %dus", pkt.raw.timestamp.seconds, pkt.raw.timestamp.micro_seconds)
		haka.log("timestamp", "packet timestamp is %s seconds", pkt.raw.timestamp:toseconds())
		haka.log("timestamp", "packet timestamp is %s", pkt.raw.timestamp)
	end
}

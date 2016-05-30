-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require("protocol/ipv4")

local icmp_dissector = haka.dissector.new{
	type = haka.helper.PacketDissector,
	name = 'icmp'
}

icmp_dissector.grammar = haka.grammar.new("icmp", function ()
	packet = record{
		field('type',     number(8)),
		field('code',     number(8)),
		field('checksum', number(16))
			:validate(function (self)
				self.checksum = 0
				self.checksum = ipv4.inet_checksum_compute(self._payload)
			end),
		field('payload',  bytes())
	}

	export(packet)
end)

function icmp_dissector.method:parse_payload(pkt, payload)
	local res = icmp_dissector.grammar.packet:parse(payload:pos("begin"))
	table.merge(self, res)
end

function icmp_dissector.method:create_payload(pkt, payload, init)
	local res = icmp_dissector.grammar.packet:create(payload:pos("begin"), init)
	table.merge(self, res)
end

function icmp_dissector.method:verify_checksum()
	return ipv4.inet_checksum_compute(self._payload) == 0
end

function icmp_dissector.method:forge_payload(pkt, payload)
	if payload.modified then
		self.checksum = nil
	end

	self:validate()
end

function icmp_dissector:create(pkt, init)
	pkt.payload:pos(0):insert(haka.vbuffer_allocate(8))
	pkt.proto = 1

	local icmp = icmp_dissector:new(pkt)
	icmp:create(init, pkt)
	return icmp
end

haka.policy {
	name = "icmp",
	on = haka.dissectors.ipv4.policies.next_dissector,
	proto = 1,
	action = haka.dissectors.icmp.install
}

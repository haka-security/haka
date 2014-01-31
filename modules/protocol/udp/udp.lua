-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require("protocol/ipv4")

local udp_dissector = haka.dissector.new{
	type = haka.dissector.EncapsulatedPacketDissector,
	name = 'udp'
}

udp_dissector.grammar = haka.grammar.record{
	haka.grammar.field('srcport',     haka.grammar.number(16)),
	haka.grammar.field('dstport',     haka.grammar.number(16)),
	haka.grammar.field('length',     haka.grammar.number(16)):
		validate(function (self) self.len = (8 + #self.payload) end),
	haka.grammar.field('checksum', haka.grammar.number(16)),
	haka.grammar.field('payload',  haka.grammar.bytes())
}:compile()

function udp_dissector.method:parse_payload(pkt, payload, init)
	self.ip = pkt
	udp_dissector.grammar:parseall(payload, self, init)
end

function udp_dissector.method:forge_payload(pkt, payload)
	if payload.modified then
		self.checksum = nil
	end

	self:validate()
end

function udp_dissector:create(pkt, init)
	if not init then init = {} end
	if not init.length then init.length = 8 end
	pkt.payload:append(haka.vbuffer(init.length))
	pkt.proto = 17

	local udp = udp_dissector:new(pkt)
	udp:parse(pkt, init)
	return udp
end

ipv4.register_protocol(17, udp_dissector)

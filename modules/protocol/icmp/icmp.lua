
local ipv4 = require("protocol/ipv4")

local icmp_dissector = haka.dissector.new{
	type = haka.dissector.EncapsulatedPacketDissector,
	name = 'icmp'
}

icmp_dissector.grammar = haka.grammar.record{
	haka.grammar.field('type',     haka.grammar.number(8)),
	haka.grammar.field('code',     haka.grammar.number(8)),
	haka.grammar.field('checksum', haka.grammar.number(16))
		:validate(function (self)
			self.checksum = 0
			self.checksum = ipv4.inet_checksum(self._payload)
		end),
	haka.grammar.field('payload',  haka.grammar.bytes())
}:compile()

function icmp_dissector.method:parse_payload(pkt, payload, init)
	self.ip = pkt
	icmp_dissector.grammar:parseall(payload, self, init)
end

function icmp_dissector.method:verify_checksum()
	return ipv4.inet_checksum(self._payload) == 0
end

function icmp_dissector.method:forge_payload(pkt, payload)
	if payload.modified then
		self.checksum = nil
	end

	self:validate()
end

function icmp_dissector:create(pkt, init)
	pkt.payload:insert(0, haka.vbuffer(8))
	pkt.proto = 1

	local icmp = icmp_dissector:new(pkt)
	icmp:parse(pkt, init)
	return icmp
end

ipv4.register_protocol(1, icmp_dissector)

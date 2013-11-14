
local ipv4 = require("protocol/ipv4")


local icmp_dissector = haka.dissector.new{
	type = haka.dissector.PacketDissector,
	name = 'icmp'
}

icmp_dissector.grammar = haka.grammar.record{
	haka.grammar.integer(1):as('type'),
	haka.grammar.integer(1):as('code'),
	haka.grammar.integer(2):as('checksum'),
	haka.grammar.bytes():as('payload')
}

icmp_dissector.grammar = icmp_dissector.grammar:compile()

function icmp_dissector.receive(pkt)
	local icmp = icmp_dissector:new(pkt)
	icmp:emit()
end

function icmp_dissector.method:__init(pkt)
	self.ip = pkt
	self._payload = pkt.payload
	icmp_dissector.grammar:parseall(self._payload:iter(), self)
end


function icmp_dissector.method:verify_checksum()
	return ipv4.inet_checksum(self._payload) == 0
end

function icmp_dissector.method:compute_checksum()
	self.checksum = 0
	self.checksum = ipv4.inet_checksum(self._payload)
end

function icmp_dissector.method:continue()
	return self.ip:continue()
end

function icmp_dissector.method:drop()
	return self.ip:drop()
end

function icmp_dissector.method:emit()
	if not haka.pcall(haka.context.signal, haka.context, self, icmp_dissector.events.receive_packet) then
		return self:drop()
	end

	if not self:continue() then
		return
	end

	return self:send()
end

local function icmp_dissector_build(icmp)
	if icmp._payload.modified then
		icmp:compute_checksum()
	end
end

function icmp_dissector.method:send()
	if not haka.pcall(haka.context.signal, haka.context, self, icmp_dissector.events.send_packet) then
		return self:drop()
	end

	if not self:continue() then
		return
	end

	icmp_dissector_build(self)
	self.ip:send()
end

function icmp_dissector.method:inject()
	icmp_dissector_build(self)
	self.ip:inject()
end

function icmp_dissector.create(pkt)
	pkt.payload:insert(0, haka.vbuffer(8))
	pkt.proto = 1
	return icmp_dissector:new(pkt)
end

ipv4.register_protocol(1, icmp_dissector)

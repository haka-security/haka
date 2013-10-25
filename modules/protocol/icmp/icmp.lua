
local ipv4 = require("protocol/ipv4")


local icmp_dissector = haka.dissector.new{
	type = haka.dissector.PacketDissector,
	name = 'icmp'
}

function icmp_dissector.receive(pkt)
	local icmp = icmp_dissector:new(pkt)
	icmp:emit()
end

function icmp_dissector.method:__init(pkt)
	self.pkt = pkt
	self.payload = pkt.payload
end

function icmp_dissector.property.get:type()
	return self.payload:sub(0, 1):asnumber('big')
end

function icmp_dissector.property.set:type(num)
	return self.payload:sub(0, 1):setnumber(num, 'big')
end

function icmp_dissector.property.get:code()
	return self.payload:sub(1, 1):asnumber('big')
end

function icmp_dissector.property.set:code(num)
	return self.payload:sub(1, 1):setnumber(num, 'big')
end

function icmp_dissector.property.get:checksum()
	return self.payload:sub(2, 2):asnumber('big')
end

function icmp_dissector.property.set:checksum(num)
	return self.payload:sub(2, 2):setnumber(num, 'big')
end

function icmp_dissector.method:verify_checksum()
	return ipv4.inet_checksum(self.pkt.payload) == 0
end

function icmp_dissector.method:compute_checksum()
	self.checksum = 0
	self.checksum = ipv4.inet_checksum(self.pkt.payload)
end

function icmp_dissector.method:continue()
	return self.pkt:continue()
end

function icmp_dissector.method:drop()
	return self.pkt:drop()
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
	if icmp.payload.modified then
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
	self.pkt:send()
end

function icmp_dissector.method:inject()
	icmp_dissector_build(self)
	self.pkt:inject()
end

ipv4.register_protocol(1, icmp_dissector)

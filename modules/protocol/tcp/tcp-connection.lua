
require("protocol/ipv4")
require("protocol/tcp")

local tcp_connection_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'tcp-connection'
}

tcp_connection_dissector:register_event(
	'new_connection',
	function (self) return self:continue() end
)

function tcp_connection_dissector.receive(pkt)
	local connection, direction, dropped = pkt:getconnection()
	if not connection then
		if pkt.flags.syn and not pkt.flags.ack then
			if not haka.pcall(haka.context.signal, haka.context, pkt, tcp_connection_dissector.events.new_connection) then
				return pkt:drop()
			end

			if not pkt:continue() then
				return
			end

			connection = pkt:newconnection()
			connection.data = haka.context.newlocal()
			connection.data:createnamespace('tcp-connection', tcp_connection_dissector:new(connection))
			direction = true
		else
			if not dropped then
				haka.log.error("tcp-connection", "no connection found")
				haka.log.debug("tcp-connection", "no connection found %s:%d -> %s:%d", pkt.ip.src,
					pkt.srcport, pkt.ip.dst, pkt.dstport)
			end
			return pkt:drop()
		end
	end

	haka.context:scope(connection.data, function ()
		local dissector = connection.data:namespace('tcp-connection')
		return dissector:emit(pkt, direction)
	end)
end

function tcp_connection_dissector.method:__init(connection)
	self.connection = connection
	self.stream = {}
	self.stream[true] = self.connection:stream(true)
	self.stream[false] = self.connection:stream(false)
	self.state = 0
end

function tcp_connection_dissector.method:emit(pkt, direction)
	local stream = self.stream[direction]
	if pkt.flags.syn then
		stream:init(pkt.seq+1)
		return pkt:send()
	elseif pkt.flags.rst then
		self:_sendpkt(pkt, direction)
		return self:drop()
	elseif self.state >= 2 then
		if pkt.flags.ack then
			self.state = self.state + 1
		end

		self:_sendpkt(pkt, direction)

		if self.state >= 3 then
			return self:close()
		else
			return
		end
	elseif pkt.flags.fin then
		self.state = self.state+1
		return self:_sendpkt(pkt, direction)
	else
		stream:push(pkt)
	end

	if stream:available() > 0 then
		if not haka.pcall(haka.context.signal, haka.context, self, tcp_connection_dissector.events.receive_data, stream, direction) then
			return self:drop()
		end
	end

	if self:continue() then
		self:_send(direction)
	end
end

function tcp_connection_dissector.method:continue()
	return self.connection ~= nil
end

function tcp_connection_dissector.method:_sendpkt(pkt, direction)
	self:_send(direction)
	
	self.stream[direction]:seq(pkt)
	self.stream[not direction]:ack(pkt)
	pkt:send()
end

function tcp_connection_dissector.method:_send(direction)
	local stream = self.stream[direction]
	local other_stream = self.stream[not direction]

	if not haka.pcall(haka.context.signal, haka.context, self, tcp_connection_dissector.events.send_data, stream, direction) then
		return self:drop()
	end

	local pkt = stream:pop()
	while pkt do
		other_stream:ack(pkt)
		pkt:send()

		pkt = stream:pop()
	end
end

function tcp_connection_dissector.method:send()
	self:_send(true)
	self:_send(false)
end

function tcp_connection_dissector.method:drop()
	self.connection:drop()
	self.stream = nil
	self.connection = nil
end

function tcp_connection_dissector.method:close()
	self.connection:close()
	self.stream = nil
	self.connection = nil
end

function tcp_connection_dissector.method:_forgereset(inv)
	local tcprst = haka.dissector.get('raw').create()
	tcprst = haka.dissector.get('ipv4').create(tcprst)

	if inv then
		tcprst.src = self.connection.dstip
		tcprst.dst = self.connection.srcip
	else
		tcprst.src = self.connection.srcip
		tcprst.dst = self.connection.dstip
	end

	tcprst.ttl = 64

	tcprst = haka.dissector.get('tcp').create(tcprst)

	if inv then
		tcprst.srcport = self.connection.dstport
		tcprst.dstport = self.connection.srcport
	else
		tcprst.srcport = self.connection.srcport
		tcprst.dstport = self.connection.dstport
	end

	tcprst.seq = self.stream[not inv].lastseq

	tcprst.flags.rst = true

	return tcprst
end

function tcp_connection_dissector.method:reset()
	local rst

	rst = self:_forgereset(false)
	rst:send()

	rst = self:_forgereset(true)
	rst:send()

	self:drop()
end

function tcp_connection_dissector.method:halfreset()
	local rst

	rst = self:_forgereset(true)
	rst:send()

	self:drop()
end

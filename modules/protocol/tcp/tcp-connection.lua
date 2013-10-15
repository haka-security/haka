
require("protocol/ipv4")
require("protocol/tcp")

local tcp_connection_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'tcp-connection'
}

tcp_connection_dissector:register_event('new_connection')

function tcp_connection_dissector.receive(pkt)
	local connection, direction, dropped = pkt:getconnection()
	if not connection then
		if pkt.flags.syn and not pkt.flags.ack then
			local data = haka.context.newscope()
			local self = tcp_connection_dissector:new()

			haka.context:scope(data, function ()
				if not haka.pcall(haka.context.signal, haka.context, self, tcp_connection_dissector.events.new_connection, pkt) then
					return pkt:drop()
				end
			end)
	
			if not pkt:continue() then
				return
			end

			connection = pkt:newconnection()
			connection.data = data
			self:init(connection)
			data:createnamespace('tcp-connection', self)
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

function tcp_connection_dissector.method:init(connection)
	self.connection = connection
	self.stream = {}
	self.stream['up'] = self.connection:stream('up')
	self.stream['down'] = self.connection:stream('down')
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
	self.stream[haka.dissector.other_direction(direction)]:ack(pkt)
	pkt:send()
end

function tcp_connection_dissector.method:_send(direction)
	local stream = self.stream[direction]
	local other_stream = self.stream[haka.dissector.other_direction(direction)]

	if not haka.pcall(haka.context.signal, haka.context, self, tcp_connection_dissector.events.send_data, stream, direction) then
		return self:drop()
	end

	local pkt = stream:pop()
	while pkt do
		if pkt:continue() then
			other_stream:ack(pkt)
			pkt:send()
		end

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

function tcp_connection_dissector.method:_forgereset(direction)
	local tcprst = haka.dissector.get('raw').create()
	tcprst = haka.dissector.get('ipv4').create(tcprst)

	if direction == 'up' then
		tcprst.src = self.connection.srcip
		tcprst.dst = self.connection.dstip
	else
		tcprst.src = self.connection.dstip
		tcprst.dst = self.connection.srcip
	end

	tcprst.ttl = 64

	tcprst = haka.dissector.get('tcp').create(tcprst)

	if direction == 'up' then
		tcprst.srcport = self.connection.srcport
		tcprst.dstport = self.connection.dstport
	else
		tcprst.srcport = self.connection.dstport
		tcprst.dstport = self.connection.srcport
	end

	tcprst.seq = self.stream[direction].lastseq

	tcprst.flags.rst = true

	return tcprst
end

function tcp_connection_dissector.method:reset()
	local rst

	rst = self:_forgereset('down')
	rst:send()

	rst = self:_forgereset('up')
	rst:send()

	self:drop()
end

function tcp_connection_dissector.method:halfreset()
	local rst

	rst = self:_forgereset('down')
	rst:send()

	self:drop()
end

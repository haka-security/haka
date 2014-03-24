-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")

local tcp_connection_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'tcp-connection'
}

tcp_connection_dissector:register_event('new_connection')
tcp_connection_dissector:register_event('receive_data', nil, haka.dissector.FlowDissector.stream_wrapper)
tcp_connection_dissector:register_event('end_connection')

function tcp_connection_dissector:receive(pkt)
	local connection, direction, dropped = pkt:getconnection()
	if not connection then
		if pkt.flags.syn and not pkt.flags.ack then
			local data = haka.context.newscope()
			local self = tcp_connection_dissector:new()

			haka.context:scope(data, function ()
				self:trigger('new_connection', pkt)
			end)

			pkt:continue()

			connection = pkt:newconnection()
			connection.data = data
			self:init(connection)
			data:createnamespace('tcp-connection', self)
		else
			if not dropped then
				haka.alert{
					severity = 'low',
					description = "no connection found for tcp packet",
					sources = {
						haka.alert.address(pkt.ip.src),
						haka.alert.service(string.format("tcp/%d", pkt.srcport))
					},
					targets = {
						haka.alert.address(pkt.ip.dst),
						haka.alert.service(string.format("tcp/%d", pkt.dstport))
					}
				}
			end
			return pkt:drop()
		end
	end

	local dissector = connection.data:namespace('tcp-connection')

	local ret, err = xpcall(function ()
		haka.context:scope(connection.data, function ()
			return dissector:emit(pkt, direction)
		end)

		if dissector._restart then
			return tcp_connection_dissector:receive(pkt)
		end
	end, debug.format_error)

	if not ret then
		if err then
			haka.log.error(dissector.name, "%s", err)
			dissector:error()
		end
	end
end

tcp_connection_dissector.states = haka.state_machine.new("tcp")

tcp_connection_dissector.states:default{
	error = function (context)
		return context.states.reset
	end,
	update = function (context, direction, pkt)
		if pkt.flags.rst then
			context.flow:_sendpkt(pkt, direction)
			return context.states.reset
		end

		if direction == context.input then
			return context.state.input(context, pkt)
		else
			assert(direction == context.output)
			return context.state.output(context, pkt)
		end
	end,
	input = function (context, pkt)
		haka.log.error('tcp-connection', "unexpected tcp packet")
		pkt:drop()
		return context.states.ERROR
	end,
	output = function (context, pkt)
		haka.log.error('tcp-connection', "unexpected tcp packet")
		pkt:drop()
		return context.states.ERROR
	end,
	finish = function (context)
		haka.pcall(haka.context.signal, haka.context, context.flow, tcp_connection_dissector.events.end_connection)
		context.flow:_close()
	end,
	reset = function (context)
		return context.states.reset
	end
}

tcp_connection_dissector.states.reset = tcp_connection_dissector.states:state{
	enter = function (context)
		context.flow:trigger('end_connection')
		context.flow.stream = nil
		context.flow.connection:drop()
	end,
	timeouts = {
		[60] = function (context)
			return context.states.FINISH
		end
	},
	finish = function (context)
		context.flow:_close()
	end
}

tcp_connection_dissector.states.initial = tcp_connection_dissector.states:state{
	init = function (context)
		context.input = 'up'
		context.output = 'down'
	end,
	input = function (context, pkt)
		if pkt.flags.syn then
			context.flow.stream[context.input]:init(pkt.seq+1)
			pkt:send()
			return context.states.syn_sent
		else 
			haka.log.error('tcp-connection', "invalid tcp establishment handshake")
			pkt:drop()
			return context.states.ERROR
		end
	end
}

tcp_connection_dissector.states.syn_sent = tcp_connection_dissector.states:state{
	output = function (context, pkt)
		if pkt.flags.syn and pkt.flags.ack then
			context.flow.stream[context.output]:init(pkt.seq+1)
			pkt:send()
			return context.states.syn_received
		else 
			haka.log.error('tcp-connection', "invalid tcp establishment handshake")
			pkt:drop()
			return context.states.ERROR
		end
	end,
	input = function (context, pkt)
		if not pkt.flags.syn then
			haka.log.error('tcp-connection', "invalid tcp establishment handshake")
			pkt:drop()
			return context.states.ERROR
		else
			pkt:send()
		end
	end
}

tcp_connection_dissector.states.syn_received = tcp_connection_dissector.states:state{
	input = function (context, pkt)
		if pkt.flags.ack then
			context.flow:push(pkt, context.input)
			return context.states.established
		elseif pkt.flags.fin then
			context.flow:push(pkt, context.input)
			return context.states.fin_wait_1
		else
			haka.log.error('tcp-connection', "invalid tcp establishment handshake")
			pkt:drop()
			return context.states.ERROR
		end
	end,
	output = function (context, pkt)
		if not pkt.flags.syn or not pkt.flags.ack then
			haka.log.error('tcp-connection', "invalid tcp establishment handshake")
			pkt:drop()
			return context.states.ERROR
		else
			context.flow:push(pkt, context.output)
		end
	end
}

tcp_connection_dissector.states.established = tcp_connection_dissector.states:state{
	input = function (context, pkt)
		if pkt.flags.fin then
			context.flow:push(pkt, context.input, true)
			return context.states.fin_wait_1
		else
			context.flow:push(pkt, context.input)
		end
	end,
	output = function (context, pkt)
		if pkt.flags.fin then
			context.flow:push(pkt, context.output, true)
			context.input, context.output = context.output, context.input
			return context.states.fin_wait_1
		else
			context.flow:push(pkt, context.output)
		end
	end
}

tcp_connection_dissector.states.fin_wait_1 = tcp_connection_dissector.states:state{
	output = function (context, pkt)
		if pkt.flags.fin then
			context.flow:finish(context.output)
			if pkt.flags.ack then
				context.flow:_sendpkt(pkt, context.output)
				return context.states.closing
			else
				context.flow:_sendpkt(pkt, context.output)
				return context.states.timed_wait
			end
		elseif pkt.flags.ack then
			context.flow:push(pkt, context.output, true)
			return context.states.fin_wait_2
		else
			haka.log.error('tcp-connection', "invalid tcp termination handshake")
			pkt:drop()
			return context.states.ERROR
		end
	end,
	input = function (context, pkt)
		if pkt.flags.fin then
			if pkt.flags.ack then
				context.flow:_sendpkt(pkt, context.input)
				return context.states.closing
			end
		end
		context.flow:_sendpkt(pkt, context.input)
	end
}

tcp_connection_dissector.states.fin_wait_2 = tcp_connection_dissector.states:state{
	output = function (context, pkt)
		if pkt.flags.fin then
			context.flow:_sendpkt(pkt, context.output)
			return context.states.timed_wait
		elseif pkt.flags.ack then
			context.flow:_sendpkt(pkt, context.output)
		else
			haka.log.error('tcp-connection', "invalid tcp termination handshake")
			pkt:drop()
			return context.states.ERROR
		end
	end,
	input = function (context, pkt)
		if pkt.flags.ack then
			context.flow:_sendpkt(pkt, context.input)
		else
			haka.log.error('tcp-connection', "invalid tcp termination handshake")
			pkt:drop()
			return context.states.ERROR
		end
	end
}

tcp_connection_dissector.states.closing = tcp_connection_dissector.states:state{
	input = function (context, pkt)
		if pkt.flags.ack then
			context.flow:_sendpkt(pkt, context.input)
			return context.states.timed_wait
		elseif not pkt.flags.fin then
			haka.log.error('tcp-connection', "invalid tcp termination handshake")
			pkt:drop()
			return context.states.ERROR
		else
			context.flow:_sendpkt(pkt, context.input)
		end
	end,
	output = function (context, pkt)
		if not pkt.flags.ack then
			haka.log.error('tcp-connection', "invalid tcp termination handshake")
			pkt:drop()
			return context.states.ERROR
		else
			context.flow:_sendpkt(pkt, context.output)
		end
	end,
}

tcp_connection_dissector.states.timed_wait = tcp_connection_dissector.states:state{
	enter = function (context)
		context.flow:trigger('end_connection')
	end,
	input = function (context, pkt)
		if pkt.flags.syn then
			context.flow:restart()
			return context.states.FINISH
		elseif not pkt.flags.ack then
			haka.log.error('tcp-connection', "invalid tcp termination handshake")
			pkt:drop()
			return context.states.ERROR
		else
			context.flow:_sendpkt(pkt, context.input)
		end
	end,
	timeouts = {
		[60] = function (context)
			return context.states.FINISH
		end
	},
	finish = function (context)
		context.flow:_close()
	end
}

function tcp_connection_dissector.method:__init()
	self.stream = {}
	self._restart = false
end

function tcp_connection_dissector.method:init(connection)
	self.connection = connection
	self.stream['up'] = self.connection:stream('up')
	self.stream['down'] = self.connection:stream('down')
	self.states = tcp_connection_dissector.states:instanciate()
	self.states.flow = self
end

function tcp_connection_dissector.method:restart()
	self.stream = nil
	self.connection:close()
	self.connection = nil
	self.states = nil
end

function tcp_connection_dissector.method:restart()
	self._restart = true
end

function tcp_connection_dissector.method:emit(pkt, direction)
	self.states:update(direction, pkt)
end

function tcp_connection_dissector.method:_close()
	self.stream = nil
	if self.connection then
		self.connection:close()
		self.connection = nil
	end
	self.states = nil
end

function tcp_connection_dissector.method:push(pkt, direction, finish)
	local stream = self.stream[direction]

	local current = stream:push(pkt)
	if finish then stream.stream:finish() end

	self:trigger('receive_data', stream.stream, current, direction)
	return self:_send(direction)
end

function tcp_connection_dissector.method:finish(direction)
	local stream = self.stream[direction]

	stream.stream:finish()

	self:trigger('receive_data', stream.stream, nil, direction)
end

function tcp_connection_dissector.method:continue()
	if not self.stream then
		haka.abort()
	end
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

	local pkt = stream:pop()
	while pkt do
		other_stream:ack(pkt)
		pkt:send()

		pkt = stream:pop()
	end
end

function tcp_connection_dissector.method:send()
	self:_send('up')
	self:_send('down')
end

function tcp_connection_dissector.method:drop()
	self.states:reset()
end

function tcp_connection_dissector.method:_forgereset(direction)
	local tcprst = haka.dissector.get('raw'):create()
	tcprst = haka.dissector.get('ipv4'):create(tcprst)

	if direction == 'up' then
		tcprst.src = self.connection.srcip
		tcprst.dst = self.connection.dstip
	else
		tcprst.src = self.connection.dstip
		tcprst.dst = self.connection.srcip
	end

	tcprst.ttl = 64

	tcprst = haka.dissector.get('tcp'):create(tcprst)

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

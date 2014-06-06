-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local raw = require("protocol/raw")
local ipv4 = require("protocol/ipv4")
local tcp = require("protocol/tcp")

local tcp_connection_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'tcp_connection'
}

tcp_connection_dissector.cnx_table = ipv4.cnx_table()

tcp_connection_dissector:register_event('new_connection')
tcp_connection_dissector:register_event('receive_data', nil, haka.dissector.FlowDissector.stream_wrapper)
tcp_connection_dissector:register_event('end_connection')

local function tcp_get_key(pkt)
	return pkt.ip.src, pkt.ip.dst, pkt.srcport, pkt.dstport
end

function tcp_connection_dissector:receive(pkt)
	local connection, direction, dropped = tcp_connection_dissector.cnx_table:get(tcp_get_key(pkt))
	if not connection then
		if pkt.flags.syn and not pkt.flags.ack then
			local data = haka.context.newscope()
			local self = tcp_connection_dissector:new()

			haka.context:exec(data, function ()
				self:trigger('new_connection', pkt)
			end)

			pkt:continue()

			connection = tcp_connection_dissector.cnx_table:create(tcp_get_key(pkt))
			connection.data = data
			self:init(connection, pkt)
			data:createnamespace('tcp_connection', self)
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

	local dissector = connection.data:namespace('tcp_connection')

	local ret, err = xpcall(function ()
		haka.context:exec(connection.data, function ()
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

tcp_connection_dissector.states = haka.state_machine("tcp", function ()
	default_transitions{
		error = function (self)
			return 'reset'
		end,
		update = function (self, direction, pkt)
			if pkt.flags.rst then
				self:_sendpkt(pkt, direction)
				return 'reset'
			end

			if direction == self.input then
				return self.state:transition('input', pkt)
			else
				assert(direction == self.output)
				return self.state:transition('output', pkt)
			end
		end,
		input = function (self, pkt)
			haka.log.error('tcp_connection', "unexpected tcp packet")
			pkt:drop()
			return 'error'
		end,
		output = function (self, pkt)
			haka.log.error('tcp_connection', "unexpected tcp packet")
			pkt:drop()
			return 'error'
		end,
		finish = function (self)
			self:trigger('end_connection')
			self:_close()
		end,
		reset = function (self)
			return 'reset'
		end
	}

	reset = state{
		enter = function (self)
			self:trigger('end_connection')
			self:clearstream()
			self.connection:drop()
		end,
		timeouts = {
			[60] = function (self)
				return 'finish'
			end
		},
		finish = function (self)
			self:_close()
		end
	}

	syn = state{
		init = function (self)
			self.input = 'up'
			self.output = 'down'
		end,
		input = function (self, pkt)
			if pkt.flags.syn then
				self.stream[self.input]:init(pkt.seq+1)
				pkt:send()
				return 'syn_sent'
			else 
				haka.log.error('tcp_connection', "invalid tcp establishment handshake")
				pkt:drop()
				return 'error'
			end
		end
	}

	syn_sent = state{
		output = function (self, pkt)
			if pkt.flags.syn and pkt.flags.ack then
				self.stream[self.output]:init(pkt.seq+1)
				pkt:send()
				return 'syn_received'
			else 
				haka.log.error('tcp_connection', "invalid tcp establishment handshake")
				pkt:drop()
				return 'error'
			end
		end,
		input = function (self, pkt)
			if not pkt.flags.syn then
				haka.log.error('tcp_connection', "invalid tcp establishment handshake")
				pkt:drop()
				return 'error'
			else
				pkt:send()
			end
		end
	}

	syn_received = state{
		input = function (self, pkt)
			if pkt.flags.ack then
				self:push(pkt, self.input)
				return 'established'
			elseif pkt.flags.fin then
				self:push(pkt, self.input)
				return 'fin_wait_1'
			else
				haka.log.error('tcp_connection', "invalid tcp establishment handshake")
				pkt:drop()
				return 'error'
			end
		end,
		output = function (self, pkt)
			if not pkt.flags.syn or not pkt.flags.ack then
				haka.log.error('tcp_connection', "invalid tcp establishment handshake")
				pkt:drop()
				return 'error'
			else
				self:push(pkt, self.output)
			end
		end
	}

	established = state{
		input = function (self, pkt)
			if pkt.flags.fin then
				self:push(pkt, self.input, true)
				return 'fin_wait_1'
			else
				self:push(pkt, self.input)
			end
		end,
		output = function (self, pkt)
			if pkt.flags.fin then
				self:push(pkt, self.output, true)
				self.input, self.output = self.output, self.input
				return 'fin_wait_1'
			else
				self:push(pkt, self.output)
			end
		end
	}

	fin_wait_1 = state{
		output = function (self, pkt)
			if pkt.flags.fin then
				self:finish(self.output)
				if pkt.flags.ack then
					self:_sendpkt(pkt, self.output)
					return 'closing'
				else
					self:_sendpkt(pkt, self.output)
					return 'timed_wait'
				end
			elseif pkt.flags.ack then
				self:push(pkt, self.output, true)
				return 'fin_wait_2'
			else
				haka.log.error('tcp_connection', "invalid tcp termination handshake")
				pkt:drop()
				return 'error'
			end
		end,
		input = function (self, pkt)
			if pkt.flags.fin then
				if pkt.flags.ack then
					self:_sendpkt(pkt, self.input)
					return 'closing'
				end
			end
			self:_sendpkt(pkt, self.input)
		end
	}

	fin_wait_2 = state{
		output = function (self, pkt)
			if pkt.flags.fin then
				self:_sendpkt(pkt, self.output)
				return 'timed_wait'
			elseif pkt.flags.ack then
				self:_sendpkt(pkt, self.output)
			else
				haka.log.error('tcp_connection', "invalid tcp termination handshake")
				pkt:drop()
				return 'error'
			end
		end,
		input = function (self, pkt)
			if pkt.flags.ack then
				self:_sendpkt(pkt, self.input)
			else
				haka.log.error('tcp_connection', "invalid tcp termination handshake")
				pkt:drop()
				return 'error'
			end
		end
	}

	closing = state{
		input = function (self, pkt)
			if pkt.flags.ack then
				self:_sendpkt(pkt, self.input)
				return 'timed_wait'
			elseif not pkt.flags.fin then
				haka.log.error('tcp_connection', "invalid tcp termination handshake")
				pkt:drop()
				return 'error'
			else
				self:_sendpkt(pkt, self.input)
			end
		end,
		output = function (self, pkt)
			if not pkt.flags.ack then
				haka.log.error('tcp_connection', "invalid tcp termination handshake")
				pkt:drop()
				return 'error'
			else
				self:_sendpkt(pkt, self.output)
			end
		end,
	}

	timed_wait = state{
		enter = function (self)
			self:trigger('end_connection')
		end,
		input = function (self, pkt)
			if pkt.flags.syn then
				self:restart()
				return 'finish'
			elseif not pkt.flags.ack then
				haka.log.error('tcp_connection', "invalid tcp termination handshake")
				pkt:drop()
				return 'error'
			else
				self:_sendpkt(pkt, self.input)
			end
		end,
		output = function (self, pkt)
			if not pkt.flags.ack then
				haka.log.error('tcp_connection', "invalid tcp termination handshake")
				pkt:drop()
				return 'error'
			else
				self:_sendpkt(pkt, self.output)
			end
		end,
		timeouts = {
			[60] = function (self)
				return 'finish'
			end
		},
		finish = function (self)
			self:_close()
		end
	}

	initial(syn)
end)

function tcp_connection_dissector.method:__init()
	self.stream = {}
	self._restart = false
end

function tcp_connection_dissector.method:init(connection, pkt)
	self.connection = connection
	self.stream['up'] = tcp.tcp_stream()
	self.stream['down'] = tcp.tcp_stream()
	self.state = tcp_connection_dissector.states:instanciate(self)
	self.srcip = pkt.ip.src
	self.dstip = pkt.ip.dst
	self.srcport = pkt.srcport
	self.dstport = pkt.dstport
end

function tcp_connection_dissector.method:clearstream()
	if self.stream then
		self.stream.up:clear()
		self.stream.down:clear()
		self.stream = nil
	end
end

function tcp_connection_dissector.method:restart()
	self._restart = true
end

function tcp_connection_dissector.method:emit(pkt, direction)
	self.connection:update_stat(direction, pkt.ip.len)

	self.state:transition('update', direction, pkt)
end

function tcp_connection_dissector.method:_close()
	self:clearstream()
	if self.connection then
		self.connection:close()
		self.connection = nil
	end
	self.state = nil
end

function tcp_connection_dissector.method:_trigger_receive(direction, stream, current)
	self:trigger('receive_data', stream.stream, current, direction)

	local next_dissector = self:next_dissector()
	if next_dissector then
		return next_dissector:receive(stream.stream, current, direction)
	else
		return self:send(direction)
	end
end

function tcp_connection_dissector.method:push(pkt, direction, finish)
	local stream = self.stream[direction]

	local current = stream:push(pkt)
	if finish then stream.stream:finish() end

	self:_trigger_receive(direction, stream, current)
end

function tcp_connection_dissector.method:finish(direction)
	local stream = self.stream[direction]

	stream.stream:finish()
	self:_trigger_receive(direction, stream, nil)
end

function tcp_connection_dissector.method:continue()
	if not self.stream then
		haka.abort()
	end
end

function tcp_connection_dissector.method:_sendpkt(pkt, direction)
	self:send(direction)

	self.stream[direction]:seq(pkt)
	self.stream[haka.dissector.other_direction(direction)]:ack(pkt)
	pkt:send()
end

function tcp_connection_dissector.method:_send(direction)
	if self.stream then
		local stream = self.stream[direction]
		local other_stream = self.stream[haka.dissector.other_direction(direction)]

		local pkt = stream:pop()
		while pkt do
			other_stream:ack(pkt)
			pkt:send()

			pkt = stream:pop()
		end
	end
end

function tcp_connection_dissector.method:send(direction)
	if not direction then
		self:_send('up')
		self:_send('down')
	else
		self:_send(direction)
	end
end

function tcp_connection_dissector.method:drop()
	self.state:transition('reset')
end

function tcp_connection_dissector.method:_forgereset(direction)
	local tcprst = raw.create()
	tcprst = ipv4.create(tcprst)

	if direction == 'up' then
		tcprst.src = self.srcip
		tcprst.dst = self.dstip
	else
		tcprst.src = self.dstip
		tcprst.dst = self.srcip
	end

	tcprst.ttl = 64

	tcprst = tcp.create(tcprst)

	if direction == 'up' then
		tcprst.srcport = self.srcport
		tcprst.dstport = self.dstport
	else
		tcprst.srcport = self.dstport
		tcprst.dstport = self.srcport
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

tcp.select_next_dissector(tcp_connection_dissector)

return {
	events = tcp_connection_dissector.events
}

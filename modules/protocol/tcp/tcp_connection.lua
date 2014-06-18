-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require("class")

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

local TcpState = class.class("TcpState", haka.state_machine.State)

function TcpState.method:__init()
	class.super(TcpState).__init(self, name)
	table.merge(self._transitions, {
		input  = {},
		output = {},
		reset  = {},
	});
end

function TcpState.method:_update(state_machine, direction, pkt)
	if pkt.flags.rst then
		state_machine._owner:_sendpkt(pkt, direction)
		state_machine:transition('reset', pkt)
		return
	end

	if direction == state_machine._owner.input then
		state_machine:transition('input', pkt)
	else
		assert(direction == state_machine._owner.output)
		state_machine:transition('output', pkt)
	end
end

tcp_connection_dissector.states = haka.state_machine.new("tcp", function ()
	state_type(TcpState)

	reset        = state()
	syn          = state()
	syn_sent     = state()
	syn_received = state()
	established  = state()
	fin_wait_1   = state()
	fin_wait_2   = state()
	closing      = state()
	timed_wait   = state()

	any:on{
		event = events.fail,
		jump = reset,
	}

	any:on{
		event = events.input,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "unexpected tcp packet")
			pkt:drop()
		end,
		jump = fail,
	}

	any:on{
		event = events.output,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "unexpected tcp packet")
			pkt:drop()
		end,
		jump = fail,
	}

	any:on{
		event = events.finish,
		action = function (self)
			self:trigger('end_connection')
			self:_close()
		end,
	}

	any:on{
		event = events.reset,
		jump = reset,
	}

	reset:on{
		event = events.enter,
		action = function (self)
			self:trigger('end_connection')
			self:clearstream()
			self.connection:drop()
		end,
	}

	reset:on{
		event = events.timeout(60),
		jump = finish,
	}

	reset:on{
		event = events.finish,
		action = function (self)
			self:_close()
		end,
	}

	syn:on{
		event = events.init,
		action = function (self)
			self.input = 'up'
			self.output = 'down'
		end,
	}

	syn:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.syn end,
		action = function (self, pkt)
			self.stream[self.input]:init(pkt.seq+1)
			pkt:send()
		end,
		jump = syn_sent,
	}

	syn:on{
		event = events.input,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp establishment handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	syn_sent:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.syn and pkt.flags.ack end,
		action = function (self, pkt)
			self.stream[self.output]:init(pkt.seq+1)
			pkt:send()
		end,
		jump = syn_received,
	}

	syn_sent:on{
		event = events.output,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp establishment handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	syn_sent:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.syn end,
		action = function (self, pkt)
			pkt:send()
		end,
	}

	syn_sent:on{
		event = events.input,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp establishment handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	syn_received:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.ack end,
		action = function (self, pkt)
			self:push(pkt, self.input)
		end,
		jump = established,
	}

	syn_received:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.fin end,
		action = function (self, pkt)
			self:push(pkt, self.input)
		end,
		jump = fin_wait_1,
	}

	syn_received:on{
		event = events.input,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp establishment handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	syn_received:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.syn and pkt.flags.ack end,
		action = function (self, pkt)
			self:push(pkt, self.output)
		end,
	}

	syn_received:on{
		event = events.output,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp establishment handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	established:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.fin end,
		action = function (self, pkt)
			self:push(pkt, self.input, true)
		end,
		jump = fin_wait_1,
	}

	established:on{
		event = events.input,
		action = function (self, pkt)
			self:push(pkt, self.input)
		end,
	}

	established:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.fin end,
		action = function (self, pkt)
			self:push(pkt, self.output, true)
			self.input, self.output = self.output, self.input
		end,
		jump = fin_wait_1,
	}

	established:on{
		event = events.output,
		action = function (self, pkt)
			self:push(pkt, self.output)
		end,
	}

	fin_wait_1:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.fin and pkt.flags.ack end,
		action = function (self, pkt)
			self:finish(self.output)
			self:_sendpkt(pkt, self.output)
		end,
		jump = closing,
	}

	fin_wait_1:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.fin end,
		action = function (self, pkt)
			self:finish(self.output)
			self:_sendpkt(pkt, self.output)
		end,
		jump = timed_wait,
	}

	fin_wait_1:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.ack end,
		action = function (self, pkt)
			self:push(pkt, self.output, true)
		end,
		jump = fin_wait_2,
	}

	fin_wait_1:on{
		event = events.output,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp termination handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	fin_wait_1:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.fin and pkt.flags.ack end,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.input)
		end,
		jump = closing,
	}

	fin_wait_1:on{
		event = events.input,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.input)
		end,
	}

	fin_wait_2:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.fin end,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.output)
		end,
		jump = timed_wait,
	}

	fin_wait_2:on{
		event = events.output,
		check = function (self, pkt) return pkt.flags.ack end,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.output)
		end,
	}

	fin_wait_2:on{
		event = events.output,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp termination handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	fin_wait_2:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.ack end,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.input)
		end,
	}

	fin_wait_2:on{
		event = events.input,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp termination handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	closing:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.ack end,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.input)
		end,
		jump = timed_wait,
	}

	closing:on{
		event = events.input,
		check = function (self, pkt) return not pkt.flags.fin end,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp termination handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	closing:on{
		event = events.input,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.input)
		end,
	}

	closing:on{
		event = events.output,
		check = function (self, pkt) return not pkt.flags.ack end,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp termination handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	closing:on{
		event = events.output,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.output)
		end,
	}

	timed_wait:on{
		event = events.enter,
		action = function (self)
			self:trigger('end_connection')
		end,
	}

	timed_wait:on{
		event = events.input,
		check = function (self, pkt) return pkt.flags.syn end,
		action = function (self, pkt)
			self:restart()
		end,
		jump = finish,
	}

	timed_wait:on{
		event = events.input,
		check = function (self, pkt) return not pkt.flags.ack end,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp termination handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	timed_wait:on{
		event = events.input,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.input)
		end,
	}

	timed_wait:on{
		event = events.output,
		check = function (self, pkt) return not pkt.flags.ack end,
		action = function (self, pkt)
			haka.log.error('tcp_connection', "invalid tcp termination handshake")
			pkt:drop()
		end,
		jump = fail,
	}

	timed_wait:on{
		event = events.output,
		action = function (self, pkt)
			self:_sendpkt(pkt, self.output)
		end,
	}

	timed_wait:on{
		event = events.timeout(60),
		jump = finish,
	}

	timed_wait:on{
		event = events.finish,
		action = function (self)
			self:_close()
		end,
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

	self.state:update(direction, pkt)
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

--
-- Console utilities
--

haka.console.tcp = {}

function haka.console.tcp.list_connections(show_dropped)
	local ret = {}
	for _, cnx in ipairs(tcp_connection_dissector.cnx_table:all(show_dropped)) do
		if cnx.data then
			local tcp_data = cnx.data:namespace('tcp_connection')

			table.insert(ret, {
				_thread = haka.current_thread(),
				_id = tcp_data.connection.id,
				id = string.format("%d-%d", haka.current_thread(), tcp_data.connection.id),
				srcip = tcp_data.srcip,
				srcport = tcp_data.srcport,
				dstip = tcp_data.dstip,
				dstport = tcp_data.dstport,
				state = tcp_data.state.current,
				in_pkts = tcp_data.connection.in_pkts,
				in_bytes = tcp_data.connection.in_bytes,
				out_pkts = tcp_data.connection.out_pkts,
				out_bytes = tcp_data.connection.out_bytes
			})
		end
	end
	return ret
end

function haka.console.tcp.drop_connection(id)
	local tcp_conn = tcp_connection_dissector.cnx_table:get_byid(id)
	if not tcp_conn then
		error("unknown tcp connection")
	end

	if tcp_conn.data then
		local tcp_data = tcp_conn.data:namespace('tcp_connection')
		tcp_data:drop()
	end
end

function haka.console.tcp.reset_connection(id)
	local tcp_conn = tcp_connection_dissector.cnx_table:get_byid(id)
	if not tcp_conn then
		error("unknown tcp connection")
	end

	if tcp_conn.data then
		local tcp_data = tcp_conn.data:namespace('tcp_connection')
		tcp_data:reset()
	end
end

return {
	events = tcp_connection_dissector.events
}

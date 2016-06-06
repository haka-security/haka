-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require("class")
local check = require("check")

local ipv4 = require("protocol/ipv4")
local tcp = require("protocol/tcp")

local module = {}
local log = haka.log_section("tcp")

local tcp_connection_dissector = haka.dissector.new{
	type = haka.helper.PacketDissector,
	name = 'tcp_connection'
}

tcp_connection_dissector.cnx_table = ipv4.cnx_table()

tcp_connection_dissector:register_event('new_connection')
tcp_connection_dissector:register_event('receive_packet')
tcp_connection_dissector:register_streamed_event('receive_data')
tcp_connection_dissector:register_event('end_connection')

tcp_connection_dissector.policies.no_connection_found = haka.policy.new("no connection found for tcp packet")
tcp_connection_dissector.policies.unexpected_packet = haka.policy.new("unexpected tcp packet")
tcp_connection_dissector.policies.invalid_handshake = haka.policy.new("invalid tcp handshake")
tcp_connection_dissector.policies.new_connection = haka.policy.new("new connection")

haka.policy {
	on = tcp_connection_dissector.policies.no_connection_found,
	name = "default action",
	action = {
		haka.policy.alert{ severity = 'low' },
		haka.policy.drop
	}
}

local function tcp_get_key(pkt)
	return pkt.src, pkt.dst, pkt.srcport, pkt.dstport
end

function tcp_connection_dissector:receive(pkt)
	local connection, direction, dropped = tcp_connection_dissector.cnx_table:get(tcp_get_key(pkt))
	if not connection then
		if pkt.flags.syn and not pkt.flags.ack then
			connection = tcp_connection_dissector.cnx_table:create(tcp_get_key(pkt))
			connection.data = haka.context.newscope()
			local self = tcp_connection_dissector:new(connection, pkt)

			local ret, err = xpcall(function ()
					haka.context:exec(connection.data, function ()
						self:trigger('new_connection', pkt)
						class.classof(self).policies.next_dissector:apply{
							values = self:install_criterion(),
							ctx = self,
						}
						self:activate_next_dissector()
					end)
				end, debug.format_error)

			if err then
				log.error("%s", err)
				pkt:drop()
				self:error()
				haka.abort()
			end

			if not pkt:can_continue() then
				self:drop()
				haka.abort()
			end

			if not self._stream then
				pkt:drop()
				haka.abort()
			end

			connection.data:createnamespace('tcp_connection', self)
		else
			if not dropped then
				tcp_connection_dissector.policies.no_connection_found:apply{
					ctx = pkt,
					values = {
						srcip = pkt.src,
						srcport = pkt.srcport,
						dstip = pkt.dst,
						dstport = pkt.dstport
					},
					desc = {
						sources = {
							haka.alert.address(pkt.src),
							haka.alert.service(string.format("tcp/%d", pkt.srcport))
						},
						targets = {
							haka.alert.address(pkt.dst),
							haka.alert.service(string.format("tcp/%d", pkt.dstport))
						}
					}
				}
				if not pkt:can_continue() then
					-- packet was dropped by policy
					return
				else
					pkt:send()
				end
			else
				return pkt:drop()
			end
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
			log.error("%s", err)
			dissector:error()
		end
	end
end

function tcp_connection_dissector.method:install_criterion()
	return { port = self.dstport }
end


haka.policy {
	on = tcp_connection_dissector.policies.unexpected_packet,
	name = "default action",
	action = {
		haka.policy.alert(),
		haka.policy.drop
	}
}

haka.policy {
	on = tcp_connection_dissector.policies.invalid_handshake,
	name = "default action",
	action = {
		haka.policy.alert(),
		haka.policy.drop
	}
}

tcp_connection_dissector.state_machine = haka.state_machine.new("tcp", function ()
	state_type{
		events = { 'input', 'output', 'reset' },
		update = function (self, state_machine, direction, pkt)
			if pkt.flags.rst then
				state_machine.owner:_sendpkt(pkt, direction)
				state_machine:trigger('reset', pkt)
				return
			end

			if direction == state_machine.owner.input then
				state_machine:trigger('input', pkt)
			else
				assert(direction == state_machine.owner.output)
				state_machine:trigger('output', pkt)
			end
		end
	}

	reset        = state()
	syn          = state()
	syn_sent     = state()
	syn_received = state()
	established  = state()
	fin_wait_1   = state()
	fin_wait_2   = state()
	closing      = state()
	timed_wait   = state()

	local function unexpected_packet(self, pkt)
		tcp_connection_dissector.policies.unexpected_packet:apply{
			ctx = pkt,
			desc = {
				sources = {
					haka.alert.address(pkt.src),
					haka.alert.service(string.format("tcp/%d", pkt.srcport))
				},
				targets = {
					haka.alert.address(pkt.dst),
					haka.alert.service(string.format("tcp/%d", pkt.dstport))
				}
			}
		}
	end

	local function invalid_handshake(type)
		return function (self, pkt)
			tcp_connection_dissector.policies.unexpected_packet:apply{
				ctx = pkt,
				desc = {
					description = string.format("invalid tcp %s handshake", type),
					sources = {
						haka.alert.address(pkt.src),
						haka.alert.service(string.format("tcp/%d", pkt.srcport))
					},
					targets = {
						haka.alert.address(pkt.dst),
						haka.alert.service(string.format("tcp/%d", pkt.dstport))
					}
				}
			}
		end
	end

	local function send(dir)
		return function(self, pkt)
			self._stream[self[dir]]:init(pkt.seq+1)
			pkt:send()
		end
	end

	any:on{
		events = { events.fail, events.reset },
		jump = reset,
	}

	any:on{
		events = { events.input, events.output },
		execute = unexpected_packet,
		jump = fail,
	}

	any:on{
		event = events.finish,
		execute = function (self)
			self:trigger('end_connection')
			self:_close()
		end,
	}

	reset:on{
		event = events.enter,
		execute = function (self)
			self:trigger('end_connection')
			self:clearstream()
			self._parent:drop()
		end,
	}

	reset:on{
		event = events.timeout(60),
		jump = finish,
	}

	reset:on{
		event = events.finish,
		execute = function (self)
			self:_close()
		end,
	}

	syn:on{
		event = events.init,
		execute = function (self)
			self.input = 'up'
			self.output = 'down'
		end,
	}

	syn:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.syn end,
		execute = send('input'),
		jump = syn_sent,
	}

	syn:on{
		event = events.input,
		execute = invalid_handshake('establishement'),
		jump = fail,
	}

	syn_sent:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.syn and pkt.flags.ack end,
		execute = send('output'),
		jump = syn_received,
	}

	syn_sent:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.syn end,
		execute = function (self, pkt)
			pkt:send()
		end,
	}

	syn_sent:on{
		events = { events.output, events.input },
		execute = invalid_handshake('establishement'),
		jump = fail,
	}

	local function push(dir, finish)
		return function (self, pkt)
			self:push(pkt, self[dir], finish)
		end
	end

	syn_received:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.ack end,
		execute = push('input'),
		jump = established,
	}

	syn_received:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.fin end,
		execute = push('input'),
		jump = fin_wait_1,
	}

	syn_received:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.syn and pkt.flags.ack end,
		execute = push('output'),
	}

	syn_received:on{
		events = { events.input, events.output },
		execute = invalid_handshake('establishement'),
		jump = fail,
	}

	established:on{
		event = events.enter,
		execute = function (self, pkt)
			self._established = true
		end
	}

	established:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.fin end,
		execute = push('input', true),
		jump = fin_wait_1,
	}

	established:on{
		event = events.input,
		execute = push('input'),
	}

	established:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.fin end,
		execute = function (self, pkt)
			self:push(pkt, self.output, true)
			self.input, self.output = self.output, self.input
		end,
		jump = fin_wait_1,
	}

	established:on{
		event = events.output,
		execute = push('output'),
	}

	local function dofinish(self, pkt)
			self:finish(self.output)
			self:_sendpkt(pkt, self.output)
	end

	local function sendpkt(dir)
		return function (self, pkt)
			self:_sendpkt(pkt, self[dir])
		end
	end

	fin_wait_1:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.fin and pkt.flags.ack end,
		execute = dofinish,
		jump = closing,
	}

	fin_wait_1:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.fin end,
		execute = dofinish,
		jump = timed_wait,
	}

	fin_wait_1:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.ack end,
		execute = push('output', true),
		jump = fin_wait_2,
	}

	fin_wait_1:on{
		event = events.output,
		execute = invalid_handshake('termination'),
		jump = fail,
	}

	fin_wait_1:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.fin and pkt.flags.ack end,
		execute = sendpkt('input'),
		jump = closing,
	}

	fin_wait_1:on{
		event = events.input,
		execute = sendpkt('input'),
	}

	fin_wait_2:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.fin end,
		execute = sendpkt('output'),
		jump = timed_wait,
	}

	fin_wait_2:on{
		event = events.output,
		when = function (self, pkt) return pkt.flags.ack end,
		execute = sendpkt('output'),
	}

	fin_wait_2:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.ack end,
		execute = sendpkt('input'),
	}

	fin_wait_2:on{
		events = { events.output, events.input },
		execute = invalid_handshake('termination'),
		jump = fail,
	}

	closing:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.ack end,
		execute = sendpkt('input'),
		jump = timed_wait,
	}

	closing:on{
		event = events.input,
		when = function (self, pkt) return not pkt.flags.fin end,
		execute = invalid_handshake('termination'),
		jump = fail,
	}

	closing:on{
		event = events.input,
		execute = sendpkt('input'),
	}

	closing:on{
		event = events.output,
		when = function (self, pkt) return not pkt.flags.ack end,
		execute = invalid_handshake('termination'),
		jump = fail,
	}

	closing:on{
		event = events.output,
		execute = sendpkt('output'),
	}

	timed_wait:on{
		event = events.enter,
		execute = function (self)
			self:trigger('end_connection')
		end,
	}

	timed_wait:on{
		event = events.input,
		when = function (self, pkt) return pkt.flags.syn end,
		execute = function (self, pkt)
			self:restart()
		end,
		jump = finish,
	}

	timed_wait:on{
		events = { events.input, events.output },
		when = function (self, pkt) return not pkt.flags.ack end,
		execute = invalid_handshake('termination'),
		jump = fail,
	}

	timed_wait:on{
		event = events.input,
		execute = sendpkt('input'),
	}

	timed_wait:on{
		event = events.output,
		execute = sendpkt('output'),
	}

	timed_wait:on{
		event = events.timeout(60),
		jump = finish,
	}

	timed_wait:on{
		event = events.finish,
		execute = function (self)
			self:_close()
		end,
	}

	initial(syn)
end)

tcp_connection_dissector.auto_state_machine = false

function tcp_connection_dissector.method:__init(connection, pkt)
	class.super(tcp_connection_dissector).__init(self, connection)
	self._stream = {}
	self._restart = false

	self.srcip = pkt.src
	self.dstip = pkt.dst
	self.srcport = pkt.srcport
	self.dstport = pkt.dstport

	self._stream['up'] = tcp.tcp_stream()
	self._stream['down'] = tcp.tcp_stream()
	self._state = tcp_connection_dissector.state_machine:instanciate(self)
end

function tcp_connection_dissector.method:clearstream()
	if self._stream then
		self._stream.up:clear()
		self._stream.down:clear()
		self._stream = nil
	end
end

function tcp_connection_dissector.method:restart()
	self._restart = true
end

function tcp_connection_dissector.method:emit(pkt, direction)
	self._parent:update_stat(direction, pkt.len)
	self:trigger('receive_packet', pkt, direction)

	self._state:update(direction, pkt)
end

function tcp_connection_dissector.method:_close()
	self:clearstream()
	if self._parent then
		self._parent:close()
		self._parent = nil
	end
	self._state = nil
end

function tcp_connection_dissector.method:_trigger_receive(direction, stream, current)
	self:trigger('receive_data', stream.stream, current, direction)

	local next_dissector = self._next_dissector
	if next_dissector then
		return next_dissector:receive(stream.stream, current, direction)
	else
		return self:send(direction)
	end
end

function tcp_connection_dissector.method:push(pkt, direction, finish)
	local stream = self._stream[direction]

	local current = stream:push(pkt)
	if finish then stream.stream:finish() end

	self:_trigger_receive(direction, stream, current)
end

function tcp_connection_dissector.method:finish(direction)
	local stream = self._stream[direction]

	stream.stream:finish()
	self:_trigger_receive(direction, stream, nil)
end

function tcp_connection_dissector.method:can_continue()
	return self._stream ~= nil
end

function tcp_connection_dissector.method:_sendpkt(pkt, direction)
	self:send(direction)

	if self._stream then
		self._stream[direction]:seq(pkt)
		self._stream[haka.dissector.opposite_direction(direction)]:ack(pkt)
		pkt:send()
	else
		haka.log.warning("tcp_connection: invalid stream")
	end
end

function tcp_connection_dissector.method:_send(direction)
	if self._stream then
		local stream = self._stream[direction]
		local other_stream = self._stream[haka.dissector.opposite_direction(direction)]

		local pkt = stream:pop()
		while pkt do
			other_stream:ack(pkt)
			pkt:send()

			pkt = stream:pop()
		end
	else
		haka.log.warning("tcp_connection: invalid stream")
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

function tcp_connection_dissector.method:error()
	self:_close()
end

function tcp_connection_dissector.method:drop()
	check.assert(self._state, "connection already dropped")

	self._state:trigger('reset')
end

function tcp_connection_dissector.method:_forgereset(direction)
	local tcprst = haka.dissectors.packet.create()
	tcprst = haka.dissectors.ipv4.create(tcprst)

	if direction == 'up' then
		tcprst.src = self.srcip
		tcprst.dst = self.dstip
	else
		tcprst.src = self.dstip
		tcprst.dst = self.srcip
	end

	tcprst.ttl = 64

	tcprst = haka.dissectors.tcp.create(tcprst)

	if direction == 'up' then
		tcprst.srcport = self.srcport
		tcprst.dstport = self.dstport
	else
		tcprst.srcport = self.dstport
		tcprst.dstport = self.srcport
	end

	tcprst.seq = self._stream[direction].lastseq

	tcprst.flags.rst = true

	return tcprst
end

function tcp_connection_dissector.method:reset()
	check.assert(self._established, "cannot reset a non-established connection")

	local rst

	rst = self:_forgereset('down')
	rst:send()

	rst = self:_forgereset('up')
	rst:send()

	self:drop()
end

function tcp_connection_dissector.method:halfreset()
	check.assert(self._established, "cannot reset a non-established connection")

	local rst

	rst = self:_forgereset('down')
	rst:send()

	self:drop()
end

haka.policy {
	name = "tcp connection",
	on = haka.dissectors.tcp.policies.next_dissector,
	action = haka.dissectors.tcp_connection.install
}

haka.rule {
	on = haka.dissectors.tcp_connection.events.new_connection,
	eval = function (flow, pkt)
		tcp_connection_dissector.policies.new_connection:apply{
			ctx = flow,
			values = {
				srcip = pkt.src,
				srcport = pkt.srcport,
				dstip = pkt.dst,
				dstport = pkt.dstport
			},
			desc = {
				sources = {
					haka.alert.address(pkt.src),
					haka.alert.service(string.format("tcp/%d", pkt.srcport))
				},
				targets = {
					haka.alert.address(pkt.dst),
					haka.alert.service(string.format("tcp/%d", pkt.dstport))
				}
			}
		}
	end
}

--
-- Helpers
--

module.helper = {}

module.helper.TcpFlowDissector = class.class('TcpFlowDissector', haka.helper.FlowDissector)

function module.helper.TcpFlowDissector.method:__init(flow)
	check.assert(class.isa(flow, tcp_connection_dissector), "invalid parent dissector, must be a tcp flow")

	class.super(module.helper.TcpFlowDissector).__init(self, flow)
end

function module.helper.TcpFlowDissector.method:can_continue()
	return self._parent ~= nil
end

function module.helper.TcpFlowDissector.method:drop()
	if self._parent then
		self._parent:drop()
		self._parent = nil
	end
end

function module.helper.TcpFlowDissector.method:reset()
	if self._parent then
		self._parent:reset()
		self._parent = nil
	end
end

function module.helper.TcpFlowDissector.method:receive(stream, current, direction)
	return haka.dissector.pcall(self, function ()
		self._parent:streamed(stream, self.receive_streamed, self, current, direction)

		if self._parent then
			self._parent:send(direction)
		end
	end)
end

function module.helper.TcpFlowDissector.method:receive_streamed(iter, direction)
	assert(self._state, "no state machine defined")

	while iter:wait() do
		self._state:update(iter, direction)
		self:continue()
	end
end


--
-- Console utilities
--

module.console = {}

function module.console.list_connections(show_dropped)
	local ret = {}
	for _, cnx in ipairs(tcp_connection_dissector.cnx_table:all(show_dropped)) do
		if cnx.data then
			local tcp_data = cnx.data:namespace('tcp_connection')

			table.insert(ret, {
				_thread = haka.current_thread(),
				_id = tcp_data.id,
				id = string.format("%d-%d", haka.current_thread(), tcp_data.id),
				srcip = tcp_data.srcip,
				srcport = tcp_data.srcport,
				dstip = tcp_data.dstip,
				dstport = tcp_data.dstport,
				state = tcp_data.state,
				in_pkts = tcp_data.in_pkts,
				in_bytes = tcp_data.in_bytes,
				out_pkts = tcp_data.out_pkts,
				out_bytes = tcp_data.out_bytes
			})
		end
	end
	return ret
end

function module.console.drop_connection(id)
	local tcp_conn = tcp_connection_dissector.cnx_table:get_byid(id)
	if not tcp_conn then
		error("unknown tcp connection")
	end

	if tcp_conn.data then
		local tcp_data = tcp_conn.data:namespace('tcp_connection')
		tcp_data:drop()
	end
end

function module.console.reset_connection(id)
	local tcp_conn = tcp_connection_dissector.cnx_table:get_byid(id)
	if not tcp_conn then
		error("unknown tcp connection")
	end

	if tcp_conn.data then
		local tcp_data = tcp_conn.data:namespace('tcp_connection')
		tcp_data:reset()
	end
end

return module

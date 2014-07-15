-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require("class")
local check = require("check")

local ipv4 = require("protocol/ipv4")
local udp = require("protocol/udp")

local module = {}

local udp_connection_dissector = haka.dissector.new{
	type = haka.helper.FlowDissector,
	name = 'udp_connection'
}

udp_connection_dissector.cnx_table = ipv4.cnx_table()

udp_connection_dissector:register_event('new_connection')
udp_connection_dissector:register_event('receive_data')
udp_connection_dissector:register_event('end_connection')

local function udp_get_cnx_key(pkt)
	return pkt.ip.src, pkt.ip.dst, pkt.srcport, pkt.dstport
end

function udp_connection_dissector:receive(pkt)
	local connection, direction = udp_connection_dissector.cnx_table:get(udp_get_cnx_key(pkt))
	if not connection then
		local data = haka.context.newscope()
		local self = udp_connection_dissector:new(pkt)

		haka.context:exec(data, function ()
			self:trigger('new_connection', pkt)
		end)

		pkt:continue()

		connection = udp_connection_dissector.cnx_table:create(udp_get_cnx_key(pkt))
		connection.data = data
		self:init(connection)
		data:createnamespace('udp_connection', self)
	end

	local dissector = connection.data:namespace('udp_connection')

	local ret, err = xpcall(function ()
		haka.context:exec(connection.data, function ()
			return dissector:emit(direction, pkt)
		end)
	end, debug.format_error)

	if not ret then
		if err then
			haka.log.error(dissector.name, "%s", err)
			dissector:error()
		end
	end
end

local UdpState = haka.state_machine.new_state_type({ 'receive', 'drop' })

function UdpState.method:update(state_machine, direction, pkt)
	state_machine:trigger('receive', pkt, direction)
end

udp_connection_dissector.state_machine = haka.state_machine.new("udp", function ()
	state_type(UdpState)

	drop = state()
	established = state()

	any:on{
		event = events.fail,
		jump = drop,
	}

	any:on{
		event = events.drop,
		jump = drop,
	}

	any:on{
		event = events.finish,
		execute = function (self)
			self:trigger('end_connection')
			self.connection:close()
		end,
	}

	drop:on{
		event = events.enter,
		execute = function (self)
			self:trigger('end_connection')
			self.dropped = true
			self.connection:drop()
		end,
	}

	drop:on{
		event = events.timeout(60),
		jump = finish,
	}

	drop:on{
		event = events.receive,
		execute = function (self, pkt, direction)
			pkt:drop()
		end,
	}

	drop:on{
		event = events.finish,
		execute = function (self)
			self.connection:close()
		end
	}

	established:on{
		event = events.receive,
		execute = function (self, pkt, direction)
			self:trigger('receive_data', pkt, direction)

			local next_dissector = self:next_dissector()
			if next_dissector then
				local payload
				pkt._restore, payload = pkt.payload:select()
				return next_dissector:receive(pkt, payload, direction)
			else
				pkt:send()
			end
		end,
		jump = established,
	}

	established:on{
		event = events.timeout(60),
		jump = fail,
	}

	initial(established)
end)

udp_connection_dissector.auto_state_machine = false

function udp_connection_dissector.method:__init(pkt)
	self.dropped = false
	self.srcip = pkt.ip.src
	self.dstip = pkt.ip.dst
	self.srcport = pkt.srcport
	self.dstport = pkt.dstport
end

function udp_connection_dissector.method:init(connection)
	self.connection = connection
	self.state = udp_connection_dissector.state_machine:instanciate(self)
end

function udp_connection_dissector.method:emit(direction, pkt)
	self.connection:update_stat(direction, pkt.ip.len)

	self.state:update(direction, pkt)
end

function udp_connection_dissector.method:send(pkt, payload, clone)
	pkt._restore:restore(payload, clone or false)
	pkt:send()
end

function udp_connection_dissector.method:drop(pkt)
	if pkt then
		return pkt:drop()
	else
		return self.state:trigger('drop')
	end
end

function udp_connection_dissector.method:can_continue()
	return not self.dropped
end

udp.select_next_dissector(udp_connection_dissector)

module.events = udp_connection_dissector.events

--
-- Helpers
--

module.helper = {}

module.helper.UdpFlowDissector =  class.class('UdpFlowDissector', haka.helper.FlowDissector)

function module.helper.UdpFlowDissector.dissect(cls, flow)
	flow:select_next_dissector(cls:new(flow))
end

function module.helper.UdpFlowDissector.install_udp_rule(cls, port)
	haka.rule{
		name = string.format("install %s dissector", cls.name),
		hook = udp_connection_dissector.events.new_connection,
		eval = function (flow, pkt)
			if pkt.dstport == port then
				haka.log.debug(cls.name, string.format("selecting %s dissector on flow", cls.name))
				flow:select_next_dissector(cls:new(flow))
			end
		end
	}
end

module.helper.UdpFlowDissector.property.connection = {
	get = function (self)
		self.connection = self.flow.connection
		return self.connection
	end
}

function module.helper.UdpFlowDissector.method:__init(flow)
	check.assert(class.isa(flow, udp_connection_dissector), "invalid flow parameter")

	class.super(module.helper.UdpFlowDissector).__init(self)
	self.flow = flow
end

function module.helper.UdpFlowDissector.method:can_continue()
	return self.flow:can_continue()
end

function module.helper.UdpFlowDissector.method:receive(pkt, payload, direction)
	assert(self.state, "no state machine defined")

	self.state:update(payload:pos('begin'), direction, pkt, payload)
end

--
-- Console utilities
--

module.console = {}

function module.console.list_connections(show_dropped)
	local ret = {}
	for _, cnx in ipairs(udp_connection_dissector.cnx_table:all(show_dropped)) do
		if cnx.data then
			local udp_data = cnx.data:namespace('udp_connection')

			table.insert(ret, {
				_thread = haka.current_thread(),
				_id = udp_data.connection.id,
				id = string.format("%d-%d", haka.current_thread(), udp_data.connection.id),
				srcip = udp_data.srcip,
				srcport = udp_data.srcport,
				dstip = udp_data.dstip,
				dstport = udp_data.dstport,
				state = udp_data.state.current,
				in_pkts = udp_data.connection.in_pkts,
				in_bytes = udp_data.connection.in_bytes,
				out_pkts = udp_data.connection.out_pkts,
				out_bytes = udp_data.connection.out_bytes
			})
		end
	end
	return ret
end

function module.console.drop_connection(id)
	local udp_conn = udp_connection_dissector.cnx_table:get_byid(id)
	if not udp_conn then
		error("unknown udp connection")
	end

	if udp_conn.data then
		local udp_data = udp_conn.data:namespace('udp_connection')
		udp_data:drop()
	end
end

return module

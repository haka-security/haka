-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require("protocol/ipv4")
local udp = require("protocol/udp")

local udp_connection_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
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
			return dissector:emit(pkt, direction)
		end)
	end, debug.format_error)

	if not ret then
		if err then
			haka.log.error(dissector.name, "%s", err)
			dissector:error()
		end
	end
end

udp_connection_dissector.states = haka.state_machine("udp", function ()
	default_transitions{
		error = function (self)
			return 'drop'
		end,
		finish = function (self)
			self:trigger('end_connection')
			self.connection:close()
		end,
		drop = function (self)
			return 'drop'
		end
	}

	drop = state{
		enter = function (self)
			self:trigger('end_connection')
			self.dropped = true
			self.connection:drop()
		end,
		timeouts = {
			[60] = function (self)
				return 'finish'
			end
		},
		update = function (self, pkt, direction)
			pkt:drop()
		end,
		finish = function (self)
			self.connection:close()
		end
	}

	established = state{
		update = function (self, pkt, direction)
			self:trigger('receive_data', pkt, direction)
	
			local next_dissector = self:next_dissector()
			if next_dissector then
				local payload
				pkt._restore, payload = pkt.payload:select()
				return next_dissector:receive(pkt, payload, direction)
			else
				pkt:send()
			end

			return 'established'
		end,
		timeouts = {
			[60] = function (self)
				return 'error'
			end
		}
	}

	initial(established)
end)

function udp_connection_dissector.method:__init(pkt)
	self.dropped = false
	self.srcip = pkt.ip.src
	self.dstip = pkt.ip.dst
	self.srcport = pkt.srcport
	self.dstport = pkt.dstport
end

function udp_connection_dissector.method:init(connection)
	self.connection = connection
	self.state = udp_connection_dissector.states:instanciate(self)
end

function udp_connection_dissector.method:emit(pkt, direction)
	self.connection:update_stat(direction, pkt.ip.len)

	self.state:transition('update', pkt, direction)
end

function udp_connection_dissector.method:send(pkt, payload, clone)
	pkt._restore:restore(payload, clone or false)
	pkt:send()
end

function udp_connection_dissector.method:drop(pkt)
	if pkt then
		return pkt:drop()
	else
		return self.state:transition('drop')
	end
end

function udp_connection_dissector.method:continue()
	if self.dropped then
		haka.abort()
	end
end

udp.select_next_dissector(udp_connection_dissector)

--
-- Console utilities
--

haka.console.udp = {}

function haka.console.udp.list_connections()
	local ret = {}
	for _, cnx in ipairs(udp_connection_dissector.cnx_table:all()) do
		if cnx.data then
			local udp_data = cnx.data:namespace('udp_connection')

			table.insert(ret, {
				thread = haka.current_thread(),
				id = udp_data.connection.id,
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

function haka.console.udp.drop_connection(id)
	local udp_conn = udp_connection_dissector.cnx_table:get_byid(id)
	if not udp_conn then
		error("unknown udp connection")
	end

	if udp_conn.data then
		local udp_data = udp_conn.data:namespace('udp_connection')
		udp_data:drop()
	end
end

return {
	events = udp_connection_dissector.events
}

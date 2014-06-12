-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local list = require('list')

require('protocol/ipv4')

local UdpConnInfo = list.new('UdpCnxInfo')

UdpConnInfo.field = {
	'id', 'srcip', 'srcport', 'dstip', 'dstport', 'state',
	'in_pkts', 'in_bytes', 'out_pkts', 'out_bytes'
}

UdpConnInfo.key = 'id'

UdpConnInfo.field_format = {
	['in_pkt']    = list.formatter.unit,
	['in_bytes']  = list.formatter.unit,
	['out_pkt']   = list.formatter.unit,
	['out_bytes'] = list.formatter.unit
}

function UdpConnInfo.method:drop()
	for _,r in ipairs(self._data) do
		local id = r.id
		hakactl.remote(r.thread, function ()
			haka.console.udp.drop_connection(id)
		end)
	end
end

console.udp = {}

function console.udp.connexions(show_dropped)
	local data = hakactl.remote('all', function ()
		return haka.console.udp.list_connections(show_dropped)
	end)

	local conn = UdpConnInfo:new()
	conn:addall(data)
	return conn
end

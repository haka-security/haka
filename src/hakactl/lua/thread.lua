-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local list = require('list')

local thread_info = list.new('thread_info')

thread_info.field = {
	'id', 'status', 'recv_pkt', 'recv_bytes',
	'trans_pkt', 'trans_bytes', 'drop_pkt'
}

thread_info.key = 'id'

thread_info.field_format = {
	['recv_pkt']    = list.formatter.unit,
	['recv_bytes']  = list.formatter.unit,
	['trans_pkt']   = list.formatter.unit,
	['trans_bytes'] = list.formatter.unit,
	['drop_pkt']    = list.formatter.unit
}

thread_info.field_aggregate = {
	['id']          = list.aggregator.replace('total'),
	['recv_pkt']    = list.aggregator.add,
	['recv_bytes']  = list.aggregator.add,
	['trans_pkt']   = list.aggregator.add,
	['trans_bytes'] = list.aggregator.add,
	['drop_pkt']    = list.aggregator.add
}

function console.threads()
	local data = hakactl.remote('any', function ()
		return haka.console.threads()
	end)

	local info = thread_info:new()
	info:add(data[1])
	return info
end

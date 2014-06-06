-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local list = require('list')
local class = require('class')

local event_info = list.new('thread_info')

event_info.field = {
	'event', 'rule_count'
}

event_info.field_format = {
	['rule_count'] = list.formatter.unit,
}

event_info.field_aggregate = {
	['rule_count'] = list.aggregator.add
}

function console.events()
	local data = hakactl.remote('any', function ()
		return haka.console.events()
	end)

	local info = event_info:new()
	info:add(data[1])
	return info
end

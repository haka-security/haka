-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local list = require('list')
local class = require('class')

local EventInfo = list.new('thread_info')

EventInfo.field = {
	'event', 'listener'
}

EventInfo.field_format = {
	['listener'] = list.formatter.unit,
}

EventInfo.field_aggregate = {
	['listener'] = list.aggregator.add
}

function console.events()
	local data = hakactl.remote('any', function ()
		return haka.console.events()
	end)

	local info = EventInfo:new()
	info:add(data[1])
	return info
end

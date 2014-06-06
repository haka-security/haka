-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local list = require('list')

local rule_info = list.new('rule_info')

rule_info.field = {
	'name', 'location', 'event', 'type'
}

rule_info.field_format = {}

function console.rules()
	local data = hakactl.remote('any', function ()
		return haka.console.rules()
	end)

	local info = rule_info:new()
	info:add(data[1])
	return info
end

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local list = require('list')

local RuleInfo = list.new('rule_info')

RuleInfo.field = {
	'name', 'location', 'event', 'type'
}

RuleInfo.field_format = {
	['name'] = list.formatter.optional("<no name>")
}

function console.rules()
	local data = hakactl.remote('any', function ()
		return haka.console.rules()
	end)

	local info = RuleInfo:new()
	info:add(data[1])
	return info
end

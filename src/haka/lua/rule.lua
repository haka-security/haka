-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

function haka.rule_summary()
	local total = 0

	for event, listeners in pairs(haka.context.connections) do
		haka.log.info("core", "%d rule(s) on event '%s'", #listeners, event.name)
		total = total + #listeners
	end
	
	if total == 0 then
		haka.log.warning("core", "no registered rule\n")
	else
		haka.log.info("core", "%d rule(s) registered\n", total)
	end
end

function haka.rule(r)
	assert(r.hook, "not hook defined for rule")
	assert(class.isa(r.hook, haka.events.Event), "rule hook must be an event")
	assert(type(r.eval) == 'function', "rule eval function expected")
	assert(not r.options or type(r.options) == 'table', "rule options should be table")

	haka.context.connections:register(r.hook, r.eval, r.options or {})
end

-- Load interactive.lua as a different file to allow to compile it
-- with the debugging information
require("interactive")

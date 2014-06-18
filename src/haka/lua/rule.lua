-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local check = require('check')

local module = {}

module.rules = {}

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
	check.assert(r.hook, "not hook defined for rule")
	check.assert(class.isa(r.hook, haka.event.Event), "rule hook must be an event")
	check.assert(type(r.eval) == 'function', "rule eval function expected")
	check.assert(not r.options or type(r.options) == 'table', "rule options should be table")

	local loc = debug.getinfo(2, 'nSl')
	r.location = string.format("%s:%d", loc.short_src, loc.currentline)
	r.type = 'simple'

	table.insert(module.rules, r)

	haka.context.connections:register(r.hook, r.eval, r.options or {})
end

function haka.console.rules()
	local ret = {}
	for _, rule in pairs(module.rules) do
		table.insert(ret, {
			name=rule.name,
			event=rule.hook.name,
			location=rule.location,
			type=rule.type
		})
	end
	return ret
end

-- Load interactive.lua as a different file to allow to compile it
-- with the debugging information
require("interactive")

return module

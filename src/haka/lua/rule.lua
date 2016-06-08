-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local check = require('check')

local module = {}
local log = haka.log_section("core")

module.rules = {}

function haka.rule_summary()
	local total = 0

	for event, listeners in pairs(haka.context.connections) do
		log.info("%d rule(s) on event '%s'", #listeners, event.name)
		total = total + #listeners
	end

	if total == 0 then
		log.warning("no registered rule\n")
	else
		log.info("%d rule(s) registered\n", total)
	end
end

function haka.rule(r)
	local loc = debug.getinfo(2, 'nSl')
	r.location = string.format("%s:%d", loc.short_src, loc.currentline)

	if r.hook then
		log.warning("rule at %s uses 'hook' keyword which is deprecated and should be replaced by 'on'", r.location)
		r.on = r.hook
	end

	check.assert(type(r) == 'table', "rule parameter must be a table")
	check.assert(r.on, "no event defined for rule")
	check.assert(class.isa(r.on, haka.event.Event), "rule must bind on an event")
	check.assert(type(r.eval) == 'function', "rule eval function expected")
	check.assert(not r.options or type(r.options) == 'table', "rule options should be table")

	r.type = 'simple'

	table.insert(module.rules, r)

	haka.context.connections:register(r.on, r.eval, r.options or {})
end

function haka.console.rules()
	local ret = {}
	for _, rule in pairs(module.rules) do
		table.insert(ret, {
			name=rule.name,
			event=rule.on.name,
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

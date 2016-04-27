-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local check = require('check')
local rule = require('rule')

local log = haka.log_section("core")

local rule_group = class.class('RuleGroup')

function rule_group.method:__init(args)
	self.on = args.on
	self.name = args.name
	self.rules = {}
	self.init = args.init or function () end
	self.continue = args.continue or function () return true end
	self.final = args.final or function () end
	self.event_continue = args.on.continue
	self.options = args.options or {}
	self.type = 'group'
end

function rule_group.method:rule(r)
	check.assert(type(r) == 'table', "rule parameter must be a table")
	check.assert(type(r.eval) == 'function', "rule eval function expected")

	table.insert(self.rules, r.eval)
end

function rule_group.method:eval(...)
	self.init(...)

	for _, rule in ipairs(self.rules) do
		local ret = rule(...)

		-- Use the event continue function to raise an abort of needed
		self.event_continue(...)

		if not self.continue(ret, ...) then
			return
		end
	end

	self.final(...)

	-- Use the event continue function to raise an abort of needed
	self.event_continue(...)
end


function haka.rule_group(args)
	local loc = debug.getinfo(2, 'nSl')
	args.location = string.format("%s:%d", loc.short_src, loc.currentline)

	if args.hook then
		log.warning("rule groupe at %s uses 'hook' keyword which deprecated and should be replaced by 'on'", args.location)
		args.on = args.hook
	end

	check.assert(type(args) == 'table', "rule parameter must be a table")
	check.assert(args.on, "no event defined for rule group")
	check.assert(class.isa(args.on, haka.event.Event), "rule must bind on an event")
	check.assert(not args.init or type(args.init) == 'function', "rule group init function expected")
	check.assert(not args.continue or type(args.continue) == 'function', "rule group continue function expected")
	check.assert(not args.final or type(args.final) == 'function', "rule group final function expected")
	check.assert(not args.options or type(args.options) == 'table', "rule group options should be table")

	local group = rule_group:new(args)

	haka.context.connections:register(args.on,
		function (...) group:eval(...) end,
		args.options)

	table.insert(rule.rules, group)

	return group
end

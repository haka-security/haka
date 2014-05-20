-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local rule_group = class.class('RuleGroup')

function rule_group.method:__init(args, continue)
	assert(args.hook, "not hook defined for rule group")
	assert(class.isa(args.hook, haka.event.Event), "rule hook must be an event")
	assert(not args.init or type(args.init) == 'function', "rule group init function expected")
	assert(not args.continue or type(args.continue) == 'function', "rule group continue function expected")
	assert(not args.final or type(args.final) == 'function', "rule group final function expected")
	assert(not args.options or type(args.options) == 'table', "rule group options should be table")

	self.rules = {}
	self.init = args.init or function () end
	self.continue = args.continue or function () return true end
	self.final = args.final or function () end
	self.event_continue = continue
	self.options = args.options or {}
end

function rule_group.method:rule(eval)
	table.insert(self.rules, eval)
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
	local group = rule_group:new(args, args.hook.continue)
	haka.context.connections:register(args.hook,
		function (...) group:eval(...) end,
		args.options)
	return group
end

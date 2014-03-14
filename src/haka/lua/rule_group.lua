-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local rule_group = class('RuleGroup')

function rule_group.method:__init(args, continue)
	self.rules = {}
	self.init = args.init or function () end
	self.continue = args.continue or function () return true end
	self.fini = args.fini or function () end
	self.event_continue = continue
end

function rule_group.method:rule(eval)
	table.insert(self.rules, eval)
end

function rule_group.method:eval(...)
	self.init(...)

	for _, rule in ipairs(self.rules) do
		local ret = rule(...)

		self.event_continue(...)

		if not self.continue(ret, ...) then
			return
		end
	end

	self.fini(...)
	self.event_continue(...)
end


function haka.rule_group(args)
	if not args.hook then
		error("no hook defined for rule group")
	end

	local group = rule_group:new(args, args.hook.continue)
	haka.context.connections:register(args.hook, function (...) group:eval(...) end)
	return group
end

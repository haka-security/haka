-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local check = require('check')

local log = haka.log_section("core")

local policy = {}

local Policy = class.class('Policy')

local function criteria_sort(criteria)
	return criteria
end

function Policy.method:__init(name, actions)
	self.name = name
	self.actions = actions
	self.policies = {}
end

function Policy.method:insert(name, criteria, action)
	log.info("register policy '%s' on '%s'", name, self.name)
	debug.breakpoint()
	table.insert(self.policies, {name = name, criteria = criteria, action = action})
end

function Policy.method:apply(values, actions, ctx)
	local qualified_policy
	debug.breakpoint()
	for _, policy in pairs(self.policies) do
		local eligible = true
		for index, criterion in pairs(policy.criteria) do
			if values[index] ~= criterion then
				eligible = false
			end
		end
		if eligible then
			qualified_policy = policy
		end
	end
	if qualified_policy then
		log.info("applying policy %s", qualified_policy.name)
		qualified_policy.action(ctx)
	else
		log.info("no matching policy")
	end
end

local mt = {
	__call = function(_, p)
		check.assert(type(p) == 'table', "policy parameter must be a table")
		check.assert(p.on, "no on defined for policy")
		check.assert(class.isa(p.on, Policy), "policy on must be a Policy")
		check.assert(type(p.action) == 'function', "policy action must be a function")

		local policy = p.on
		p.on = nil
		local name = p.name
		p.name = nil
		local action = p['action']
		p.action = nil
		policy:insert(name or '', p, action)
	end
}

setmetatable(policy, mt)

function policy.new(...)
	return Policy:new(...)
end

function policy.drop(el)
	ctx:drop()
end

function policy.drop_with_alert(alert)
	return function(ctx)
		ctx:drop()
		haka.alert(alert)
	end
end

haka.policy = policy

return {}

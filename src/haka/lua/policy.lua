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

function Policy.method:insert(name, criteria, actions)
	if name then
		log.info("register policy '%s' on '%s'", name, self.name)
	else
		log.info("register policy on '%s'", self.name)
	end
	table.insert(self.policies, {name = name, criteria = criteria, actions = actions})
end

function policy.new_criterion(create, compare)
	local mt = {
		__call = compare
	}

	return function (...)
		return setmetatable(create(...), mt)
	end
end

policy.set = policy.new_criterion(
	function (list)
		local set = {}
		for _, m in pairs(list) do
			set[m] = true
		end
		return set
	end,
	function (self, value)
		return self[value] == true
	end
)

policy.range = policy.new_criterion(
	function (min, max)
		check.assert(min <= max, "invalid bounds")
		return { min=min, max=max }
	end,
	function (self, value) return value >= self.min and value <= self.max end
)

policy.outofrange = policy.new_criterion(
	function (min, max)
		check.assert(min <= max, "invalid bounds")
		return { min=min, max=max }
	end,
	function (self, value) return value < self.min or value > self.max end
)

function Policy.method:apply(p)
	check.assert(type(p) == 'table', "policy parameter must be a table")
	check.assert(p.ctx, "no context defined for policy")
	if p.values then
		check.assert(type(p.values) == 'table', "values must be a table")
	end
	if p.desc then
		check.assert(type(p.desc) == 'table', "desc must be a table")
	else
		p.desc = {}
	end

	-- Evaluate in given order and take action on the *last* eligible policy
	local qualified_policy
	for _, policy in pairs(self.policies) do
		local eligible = true
		for index, criterion in pairs(policy.criteria) do
			if type(criterion) == 'table' then
				if not criterion(p.values[index]) then
					eligible = false
					break
				end
			elseif p.values[index] ~= criterion then
				eligible = false
				break
			end
		end
		if eligible then
			qualified_policy = policy
		else
			log.debug("rejected policy %s for %s", policy.name or "<unnamed>", self.name)
		end
	end
	if qualified_policy then
		if qualified_policy.name then
			log.debug("applying policy %s for %s", qualified_policy.name, self.name)
		else
			log.debug("applying policy for %s", self.name)
		end

		for _,action in ipairs(qualified_policy.actions) do
			action(self, p.ctx, p.values, p.desc)
		end
	end
end

local mt = {
	__call = function(_, p)
		check.assert(type(p) == 'table', "policy parameter must be a table")
		check.assert(p.on, "no on defined for policy")
		check.assert(class.isa(p.on, Policy), "policy on must be a Policy")

		local actions = p.action

		if type(p.action) == 'table' then
			for _, func in ipairs(p.action) do
				check.assert(type(func) == 'function',
					"policy action must be a function or a list of functions")
			end
		else
			check.assert(type(p.action) == 'function',
				"policy action must be a function or a list of functions")

			actions = { p.action }
		end

		local policy = p.on
		p.on = nil
		local name = p.name
		p.name = nil
		p.action = nil
		policy:insert(name, p, actions)
	end
}

setmetatable(policy, mt)

function policy.new(...)
	return Policy:new(...)
end

function policy.drop(policy, ctx, values, desc)
	ctx:drop()
end

function policy.alert(_alert)
	return function(policy, ctx, values, desc)
		local alert = {}
		for k, v in pairs(_alert) do
			alert[k] = v
		end
		for k, v in pairs(desc) do
			if not alert[k] then
				alert[k] = v
			end
		end
		if not alert.description then
			alert.description = policy.name
		end
		haka.alert(alert)
	end
end

function policy.accept(policy, ctx, values, desc)
	-- Nothing to do
end

function policy.log(section, level, message, ...)
	local args = {...}
	return function (policy, ctx, values, desc)
		section[level](message, unpack(args))
	end
end

haka.policy = policy

return {}

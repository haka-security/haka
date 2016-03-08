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
	if name then
		log.info("register policy '%s' on '%s'", name, self.name)
	else
		log.info("register anonymous policy on '%s'", self.name)
	end
	table.insert(self.policies, {name = name, criteria = criteria, action = action})
end

function policy.set(list)
	local set = {}
	for _,m in pairs(list) do
		set[m] = true
	end
	return set
end

function policy.range(min,max)
	return { min = min, max = max }
end

function policy.outofrange(min, max)
	return { lessthan = min, morethan = max }
end

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

	local qualified_policy
	-- Evaluate in given order and take action on the *last* eligible policy
	for _, policy in pairs(self.policies) do
		local eligible = true
		for index, criterion in pairs(policy.criteria) do
			if type(criterion) == 'table' then
				if criterion.min ~= nil and criterion.max ~= nil then
					if p.values[index] < criterion.min or p.values[index] > criterion.max then
						eligible = false
						break
					end
				elseif criterion.lessthan ~= nil and criterion.morethan ~= nil then
					if p.values[index] > criterion.lessthan and p.values[index] < criterion.morethan then
						eligible = false
						break
					end
				elseif criterion[p.values[index]] == nil then
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
		end
	end
	if qualified_policy then
		if qualified_policy.name then
			log.info("applying policy %s", qualified_policy.name)
		else
			log.info("applying anonymous policy for %s", self.name)
		end
		qualified_policy.action(self, p.ctx, p.values, p.desc)
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
		policy:insert(name, p, action)
	end
}

setmetatable(policy, mt)

function policy.new(...)
	return Policy:new(...)
end

function policy.drop(el)
	ctx:drop()
end

function policy.drop_with_alert(_alert)
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
		ctx:drop()
		haka.alert(alert)
	end
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

haka.policy = policy

return {}

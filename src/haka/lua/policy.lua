-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local check = require('check')

local log = haka.log_section("core")

local module = {}

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
	-- JIT Optim
	local linear_criteria = {}
	for index, criterion in pairs(criteria) do
		if not class.isa(criterion, module.Criterion) then
			criterion = module.value(criterion)
		end
		table.insert(linear_criteria, {name = index, value = criterion})
	end
	table.insert(self.policies, {name = name, criteria = linear_criteria, actions = actions})
end

module.Criterion = class.class('Criterion')

function module.Criterion.__class_init(self, cls)
	self.super:__class_init(cls)

	module[cls.name] = {}
	setmetatable(module[cls.name], {
		__class = cls,
		__call = function (table, ...)
			local self = cls:new()
			self:init(...)
			return self
		end
	})
end

function module.Criterion.method:init()
	self._learn = nil
end

function module.Criterion.method:compare()
	error("not implemented")
end

function module.Criterion.method:learn()
	error("not implemented")
end

local ValueCriterion = class.class('value', module.Criterion)

function ValueCriterion.method:init(value)
	self._value = value
end

function ValueCriterion.method:compare(value)
	if value == nil then
		return false
	end
	return self._value == value
end

local SetCriterion = class.class('set', module.Criterion)

function SetCriterion.method:__init(list)
	self._set = {}
end

function SetCriterion.method:init(list)
	for _, m in pairs(list) do
		self._set[m] = true
	end
end

function SetCriterion.method:compare(value)
	if value == nil then
		return false
	end
	return self._set[value] == true
end

function SetCriterion.method:learn(value)
	if value == nil then
		return
	end

	self._set[value] = true
end

local RangeCriterion = class.class('range', module.Criterion)

function RangeCriterion.method:init(min, max)
	check.assert(min <= max, "invalid bounds")
	self._min = min
	self._max = max
end

function RangeCriterion.method:compare(value)
	if value == nil or self._min == nil or self._max == nil then
		return false
	end
	return value >= self._min and value <= self._max
end

function RangeCriterion.method:learn(value)
	if value == nil then
		return
	end

	if self._min == nil or value < self._min then
		self._min = value
	end

	if self._max == nil or value > self._max then
		self._max = value
	end
end

local OutofrangeCriterion = class.class('outofrange', RangeCriterion)

function OutofrangeCriterion.method:compare(value)
	if value == nil then
		return false
	end
	return value < self._min or value > self._max
end

local PresentCriterion = class.class('present', module.Criterion)

function PresentCriterion.method:compare(value)
	return value ~= nil
end

local AbsentCriterion = class.class('absent', module.Criterion)

function AbsentCriterion.method:compare(value)
	return value == nil
end

function module.learn(criterion)
	local mt = getmetatable(criterion)
	check.assert(mt and class.isaclass(mt.__class, module.Criterion), "can only learn on haka.policy.Criterion")
	local cls = mt.__class
	check.assert(cls.__method.learn, "cannot learn on haka.policy." .. cls.name .. " criterion")

	local instance = cls:new()
	instance._learn = true
	return instance
end

module.learning = false

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
	for _, policy in ipairs(self.policies) do
		local eligible = true
		local learning_policy = false
		for _, criterion in ipairs(policy.criteria) do
			local index = criterion.name
			local criterion = criterion.value
			if module.learning and criterion._learn then
				learning_policy = true
			elseif not criterion:compare(p.values[index]) then
				eligible = false
				break
			end
		end

		if eligible then
			if learning_policy then
				log.debug("learning policy %s for %s", policy.name or "<unnamed>", self.name)
				for _, criterion in ipairs(policy.criteria) do
					local index = criterion.name
					local criterion = criterion.value
					if criterion._learn then
						criterion:learn(p.values[index])
					end
				end
			end
			qualified_policy = policy
		else
			log.debug("rejected policy %s for %s", policy.name or "<unnamed>", self.name)
		end
	end

	if qualified_policy then
		if qualified_policy.name then
			log.debug("applying policy %s for %s", qualified_policy.name, self.name)
		else
			log.debug("applying anonymous policy for %s", self.name)
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

setmetatable(module, mt)

function module.new(...)
	return Policy:new(...)
end

function module.drop(policy, ctx, values, desc)
	ctx:drop()
end

function module.alert(_alert)
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

function module.accept(policy, ctx, values, desc)
	-- Nothing to do
end

function module.log(logf, message, ...)
	local args = {...}
	return function (policy, ctx, values, desc)
		logf(message, unpack(args))
	end
end

haka.policy = module

return {}

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local dg = require('grammar_dg')

local module = {}

--
-- State
--
module.State = class.class('State')

function module.State.method:__init(g)
	assert(class.isa(g, dg.Entity), "state expect an exported element of a grammar")
	self._grammar = g
	self._transitions = {
		error = {},
		enter = {},
		leave = {},
		init = {},
		finish = {},
		up = {},
		down = {},
	}
	self._name = '<unnamed>'
end

function module.State.method:on(transition)
	if not self._transitions[transition.event] then
		error("Unknown event")
	end

	-- TODO assert there is either an action or a jump otherwise transition
	-- is useless

	if transition.action then
		assert(type(transition.action) == 'function', "action must be a function")
	end

	if transition.check then
		assert(type(transition.check) == 'function', "check must be a function")
	end

	-- build another representation of the transition
	local t = {
		check = transition.check,
		action = transition.action,
	}

	if transition.jump then
		t.jump = transition.jump._name
	end

	table.insert(self._transitions[transition.event], t)
end

function module.State.method:_update(state_machine, payload, direction, pkt)
	local res, err = self._grammar:parse(payload:pos('begin'))
	if err then
		haka.alert{
			description = string.format("invalid protocol %s", err.rule),
			severity = 'low'
		}
		self.flow:drop(pkt)
	else
		state_machine:transition(direction, payload, direction, pkt)
	end
end

--
-- CompiledState
--
module.CompiledState = class.class('CompiledState')

local function transitions_wrapper(state_table, transitions, ...)
	for _, t in ipairs(transitions) do
		if not t.check or t.check(...) then
			if t.action then
				t.action(...)
			end
			if t.jump then
				newstate = state_table[t.jump]
				if not newstate then
					error(string.format("unknown state '%s'", newstate))
				end

				return newstate._compiled_state
			end
		end
	end
end

local function build_transitions_wrapper(state_table, transitions)
	return function (...)
		return transitions_wrapper(state_table, transitions, ...)
	end
end

function module.CompiledState.method:__init(state_machine, state, name)
	self._compiled_state = state_machine._state_machine:create_state(name)
	self._name = name
	self._state = state

	for n, t in pairs(state._transitions) do
		local transitions_wrapper = build_transitions_wrapper(state_machine._state_table, t)

		if n == 'fail' then
			self._compiled_state:transition_fail(transitions_wrapper)
		elseif n == 'enter' then
			self._compiled_state:transition_enter(transitions_wrapper)
		elseif n == 'leave' then
			self._compiled_state:transition_leave(transitions_wrapper)
		elseif n == 'init' then
			self._compiled_state:transition_init(transitions_wrapper)
		elseif n == 'finish' then
			self._compiled_state:transition_finish(transitions_wrapper)
		else
			self[n] = transitions_wrapper
		end
	end
end

--
-- Accessors
--

function module.basic(g)
	return module.State:new(g)
end

return module

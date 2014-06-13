-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local dg = require('grammar_dg')

local module = {}

--
-- Transition collection
--

module.TransitionCollection = class.class('TransitionCollection')

function module.TransitionCollection.method:__init()
	self._transitions = {}
end

function module.TransitionCollection.method:on(transition)
	if transition.check then
		assert(type(transition.check) == 'function', "check must be a function")
	end

	if transition.action then
		assert(type(transition.action) == 'function', "action must be a function")
	end

	if transition.jump then
		assert(class.isa(transition.jump, module.State), "can only jump on defined state")
	end

	assert(transition.timeout or transition.event, "transition must have either an event or a timeout")
	assert(transition.action or transition.jump, "transition must have either an action or a jump")

	-- build another representation of the transition
	local t = {
		check = transition.check,
		action = transition.action,
	}

	if transition.jump then
		t.jump = transition.jump._name
	end

	if not self._transitions[transition.event] then
		self._transitions[transition.event] = {}
	end
	table.insert(self._transitions[transition.event], t)
end


--
-- State
--
module.State = class.class('State', module.TransitionCollection)

function module.State.method:__init(name)
	self._transitions = {
		error = {},
		enter = {},
		leave = {},
		init = {},
		finish = {},
		up = {},
		down = {}
	}
	self._name = name or '<unnamed>'
end

function module.State.method:on(transition)
	if not self._transitions[transition.event] then
		error(string.format("unknown event '%s'", transition.event))
	end

	class.super(module.State).on(self, transition)
end

function module.State.method:setdefaults(defaults)
	assert(class.classof(defaults) == module.TransitionCollection, "can only set default with a raw TransitionCollection")
	for name, t in pairs(defaults._transitions) do
		-- Don't add transition to state that doesn't support it
		if self._transitions[name] then
			table.append(self._transitions[name], t)
		end
	end
end

function module.State.method:_update(state_machine, direction)
	state_machine:transition(direction)
end

module.GrammarState = class.class('GrammarState', module.State)

function module.GrammarState.method:__init(g, name)
	class.super(module.GrammarState).__init(self, name)
	assert(class.isa(g, dg.Entity), "state expect an exported element of a grammar")
	self._grammar = g
	self._transitions.parse_error = {}
end

function module.GrammarState.method:_update(state_machine, payload, direction, pkt)
	local res, err = self._grammar:parse(payload:pos('begin'))
	if err then
		state_machine:transition("parse_error", pkt, err)
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

function module.basic()
	return module.State:new()
end

function module.grammar(g)
	return module.GrammarState:new(g)
end

return module

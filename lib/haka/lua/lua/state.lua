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

function module.TransitionCollection.method:__init(const)
	self._const = const or false
	self._transitions = {
		timeouts = {}
	}
end

function module.TransitionCollection.method:on(transition)
	if not transition.event then
		error("transition must have an event", 2)
	end

	if not transition.event.name then
		error("transition must be a table", 2)
	end

	if self._const and not self._transitions[transition.event.name] then
		error(string.format("unknown event '%s'", transition.event.name), 2)
	end

	if transition.check and not type(transition.check) == 'function' then
		error("check must be a function", 2)
	end

	if transition.action and not type(transition.action) then
		error("action must be a function", 2)
	end

	if transition.jump and not class.isa(transition.jump, module.State) then
		error("can only jump on defined state", 2)
	end

	if not transition.action and not transition.jump then
		error("transition must have either an action or a jump", 2)
	end

	-- build another representation of the transition
	local t = {
		check = transition.check,
		action = transition.action,
	}

	if transition.jump then
		t.jump = transition.jump._name
	end

	-- register transition
	if transition.event.name == 'timeouts' then
		self._transitions.timeouts[transition.event.timeout] = t
	else
		if not self._transitions[transition.event.name] then
			self._transitions[transition.event.name] = {}
		end

		table.insert(self._transitions[transition.event.name], t)
	end

end


--
-- State
--
module.State = class.class('State', module.TransitionCollection)

function module.State.method:__init(name)
	class.super(module.State).__init(self, true)
	self._name = name or '<unnamed>'
	table.merge(self._transitions, {
		fail = {},
		enter = {},
		leave = {},
		init = {},
		finish = {},
	});
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

function module.State.method:_update(state_machine, event)
	state_machine:transition(event)
end

function module.State.method:_dump_graph(file)
	local dest = {}
	for name, transitions in pairs(self._transitions) do
		for _, t  in ipairs(transitions) do
			if t.jump then
				dest[t.jump] = true
			end
		end
	end

	for jump, _ in pairs(dest) do
		file:write(string.format('%s -> %s;\n', self._name, jump))
	end
end

module.TestState = class.class('TestState', module.State)

function module.TestState.method:__init(name)
	class.super(module.TestState).__init(self, name)
	table.merge(self._transitions, {
		test = {},
	});
end

module.BidirectionnalState = class.class('BidirectionnalState', module.State)

function module.BidirectionnalState.method:__init(gup, gdown)
	if gup and not class.isa(gup, dg.Entity) then
		error("bidirectionnal state expect an exported element of a grammar", 3)
	end

	if gdown and not class.isa(gdown, dg.Entity) then
		error("bidirectionnal state expect an exported element of a grammar", 3)
	end

	class.super(module.BidirectionnalState).__init(self)
	table.merge(self._transitions, {
		up = {},
		down = {},
		parse_error = {},
		missing_grammar = {},
	})

	self._grammar = {
		up = gup,
		down = gdown,
	}
end

function module.BidirectionnalState.method:_update(state_machine, payload, direction, ...)
	if not self._grammar[direction] then
		state_machine:transition("missing_grammar", direction, payload, ...)
	else
		local res, err = self._grammar[direction]:parse(payload, state_machine._owner)
		if err then
			state_machine:transition("parse_error", err, ...)
		else
			state_machine:transition(direction, res, ...)
		end
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
					error(string.format("unknown state '%s'", t.jump))
				end

				return newstate._compiled_state
			end
			-- return anyway since we have done this transition
			return
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

		if n == 'timeouts' then
			for timeout, transition in pairs(t) do
				self._compiled_state:transition_timeout(timeout, build_transitions_wrapper(state_machine._state_table, { transition }))
			end
		elseif n == 'fail' then
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

return module

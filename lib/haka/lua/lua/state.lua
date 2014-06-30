-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local dg = require('grammar_dg')

local module = {}

--
-- Actions collection
--

module.ActionCollection = class.class('ActionCollection')

function module.ActionCollection.method:__init(const)
	self._const = const or false
	self._actions = {
		timeouts = {}
	}
end

function module.ActionCollection.method:on(action)
	if not action.event then
		error("action must have an event", 2)
	end

	if not action.event.name then
		error("action must be a table", 2)
	end

	if self._const and not self._actions[action.event.name] then
		error(string.format("unknown event '%s'", action.event.name), 2)
	end

	if action.when and not type(action.when) == 'function' then
		error("when must be a function", 2)
	end

	if action.execute and not type(action.execute) then
		error("execute must be a function", 2)
	end

	if action.jump and not class.isa(action.jump, module.State) then
		error("can only jump on defined state", 2)
	end

	if not action.execute and not action.jump then
		error("action must have either an execute or a jump", 2)
	end

	-- build another representation of the action
	local a = {
		when = action.when,
		execute = action.execute,
	}

	if action.jump then
		a.jump = action.jump._name
	end

	-- register action
	if action.event.name == 'timeouts' then
		self._actions.timeouts[action.event.timeout] = a
	else
		if not self._actions[action.event.name] then
			self._actions[action.event.name] = {}
		end

		table.insert(self._actions[action.event.name], a)
	end

end

function module.ActionCollection.method:_register_events(events)
	if not events then
		return
	end

	for _, event in pairs(events) do
		if self._actions[event] then
			haka.log.warning("state_machine", "cannot redefine event: %s", event)
		else
			self._actions[event] = {}
		end
	end
end


--
-- State
--
module.State = class.class('State', module.ActionCollection)

module.State._events = { 'fail', 'enter', 'leave', 'init', 'finish' }

function module.State.method:__init(name)
	class.super(module.State).__init(self, true)
	self._name = name or '<unnamed>'
	local c = class.classof(self)
	while c do
		if c == module.ActionCollection then return end
		self:_register_events(c._events)
		c = c.super
	end
end

function module.State.method:setdefaults(defaults)
	assert(class.classof(defaults) == module.ActionCollection, "can only set default with a raw ActionCollection")
	for name, a in pairs(defaults._actions) do
		-- Don't add action to state that doesn't support it
		if self._actions[name] then
			table.append(self._actions[name], a)
		end
	end
end

function module.State.method:_update(state_machine, event)
	state_machine:trigger(event)
end

function module.State.method:_dump_graph(file)
	local dest = {}
	for name, actions in pairs(self._actions) do
		for _, a  in ipairs(actions) do
			if a.jump then
				dest[a.jump] = true
			end
		end
	end

	for jump, _ in pairs(dest) do
		file:write(string.format('%s -> %s;\n', self._name, jump))
	end
end

module.BidirectionalState = class.class('BidirectionalState', module.State)

module.BidirectionalState._events = { 'up', 'down', 'parse_error', 'missing_grammar' }

function module.BidirectionalState.method:__init(gup, gdown)
	if gup and not class.isa(gup, dg.Entity) then
		error("bidirectionnal state expect an exported element of a grammar", 3)
	end

	if gdown and not class.isa(gdown, dg.Entity) then
		error("bidirectionnal state expect an exported element of a grammar", 3)
	end

	class.super(module.BidirectionalState).__init(self)

	self._grammar = {
		up = gup,
		down = gdown,
	}
end

function module.BidirectionalState.method:_update(state_machine, payload, direction, ...)
	if not self._grammar[direction] then
		state_machine:trigger("missing_grammar", direction, payload, ...)
	else
		local res, err = self._grammar[direction]:parse(payload, state_machine.owner)
		if err then
			state_machine:trigger("parse_error", err, ...)
		else
			state_machine:trigger(direction, res, ...)
		end
	end
end

--
-- CompiledState
--
module.CompiledState = class.class('CompiledState')

local function transitions_wrapper(state_table, actions, ...)
	for _, a in ipairs(actions) do
		if not a.when or a.when(...) then
			if a.execute then
				a.execute(...)
			end
			if a.jump then
				newstate = state_table[a.jump]
				if not newstate then
					error(string.format("unknown state '%s'", a.jump))
				end

				return newstate._compiled_state
			end
			-- return anyway since we have done this action
			return
		end
	end
end

local function build_transitions_wrapper(state_table, actions)
	return function (...)
		return transitions_wrapper(state_table, actions, ...)
	end
end

function module.CompiledState.method:__init(state_machine, state, name)
	self._compiled_state = state_machine._state_machine:create_state(name)
	self._name = name
	self._state = state

	for n, a in pairs(state._actions) do
		local transitions_wrapper = build_transitions_wrapper(state_machine._state_table, a)

		if n == 'timeouts' then
			for timeout, action in pairs(a) do
				self._compiled_state:transition_timeout(timeout, build_transitions_wrapper(state_machine._state_table, { action }))
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

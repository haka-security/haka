-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local state = require("state")

-- Hide internals
local state_machine = haka.state_machine
local state_machine_state = haka.state
local state_machine_instance = haka.state_machine_instance
haka.state_machine = nil
haka.state = nil
haka.state_machine_instance = nil

local class = require('class')

local states = {}

states.StateMachine = class.class('StateMachine')

function states.StateMachine.method:__init(name)
	self._states = {}
	self.name = name
end

function states.StateMachine.method:compile()
	if not self._state_machine then
		self._state_machine = state_machine(self.name)
		self._state_table = {}

		local initial

		if not self._initial then
			error("state machine must have an initial state")
		end

		for name, s in pairs(self._states) do
			if class.isa(s, state.State) then
				local new_state = state.CompiledState:new(self, s, name)
				self._state_table[name] = new_state

				if s == self._initial then
					initial = new_state
				end
			end
		end

		if not initial then
			error("state machine has an unknown initial state")
		end

		self._state_machine.initial = initial._compiled_state
		self._state_table.initial = initial
		self._state_table.fail = { _compiled_state = self._state_machine.fail_state }
		self._state_table.finish = { _compiled_state = self._state_machine.finish_state }

		self._state_machine:compile()
	end
end

function states.StateMachine.method:instanciate(owner)
	return states.StateMachineInstance:new(self, owner)
end

states.StateMachineInstance = class.class('StateMachineInstance')

states.StateMachineInstance.property.current = {
	get = function (self)
		return self._instance.state
	end
}

function states.StateMachineInstance.method:__init(state_machine, owner)
	assert(state_machine, "state machine description required")
	assert(owner, "state machine context required")

	self._state_machine = state_machine
	self.states = state_machine._state_table
	self._instance = state_machine._state_machine:instanciate(owner)
	self._instance:init()
	self._owner = owner
end

function states.StateMachineInstance.method:update(...)
	local current = self.states[self._instance.state]
	if not current then
		error("state machine instance has finished")
	end

	current._state:_update(self, ...)
end

function states.StateMachineInstance.method:transition(name, ...)
	local current = self.states[self._instance.state]
	if not current then
		error("state machine instance has finished")
	end

	local trans = current[name]
	if not trans then
		error(string.format("no transition named '%s' on state '%s'", name, current._name))
	end

	local newstate = trans(self._owner, ...)

	if newstate then
		self._instance:update(newstate)
	end
end

function states.StateMachineInstance.method:finish()
	self._instance:finish()
end


local state_machine_int = {}

local function state_machine_env(state_machine)
	state_machine_int.initial = function (state)
		state_machine._initial = state
	end

	-- Fake special states
	state_machine_int.finish = {
		_name = "finish"
	}

	state_machine_int.fail = {
		_name = "fail"
	}

	-- Link to state
	state_machine_int.state = state

	-- Events proxying
	state_machine_int.events = {}
	setmetatable(state_machine_int.events, {
		__index = function (self, name)
			return name
		end,
		__newindex = function ()
			error("events is a read only table")
		end
	})

	return {
		__index = function (self, name)
			local ret

			-- Search in the state_machine environment
			ret = state_machine_int[name]
			if ret then return ret end

			-- Search the defined rules
			ret = state_machine._states[name]
			if ret then return ret end

			return nil
		end,
		__newindex = function (self, key, value)
			-- Forbid override to state_machine elements
			if state_machine_int[key] then
				error(string.format("'%s' is reserved in the state machine scope", key))
			end

			if class.isa(value, state.State) then
				-- Add the object in the states
				assert(class.isa(value, state.State), "declared state must be of type State")
				value._name = key
				state_machine._states[key] = value
			else
				rawset(self, key, value)
			end
		end
	}
end

function haka.state_machine(name, def)
	assert(type(def) == 'function', "state machine definition must by a function")

	local state_machine = states.StateMachine:new(name)

	-- Add a metatable to the environment only during the definition
	-- of the grammar.
	local env = debug.getfenv(def)
	setmetatable(env, state_machine_env(state_machine))

	def()
	setmetatable(env, nil)

	state_machine:compile()
	return state_machine
end

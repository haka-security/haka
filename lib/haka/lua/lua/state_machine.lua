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
	self._states = { }
	self._default = state.ActionCollection:new()
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

		local unused = {}
		for name, s in pairs(self._states) do
			unused[name] = s
		end

		for name, s in pairs(self._states) do
			assert(class.isa(s, state.State), "invalid state type")
			s:setdefaults(self._default)
			local new_state = state.CompiledState:new(self, s, name)
			self._state_table[name] = new_state

			if s == self._initial then
				initial = new_state
				unused[name] = nil
			end

			-- Mark states as used
			for _, event in pairs(s._actions) do
				for _, a in ipairs(event) do
					if a.jump then
						unused[a.jump] = nil
					end
				end
			end
		end

		if not initial then
			error("state machine has an unknown initial state")
		end

		for name, _ in pairs(unused) do
			haka.log.warning("state_machine", "state machine %s never jump on state: %s", self.name, name)
		end

		self._state_machine.initial = initial._compiled_state
		self._state_table.initial = initial
		self._state_table.fail = { _compiled_state = self._state_machine.fail_state }
		self._state_table.finish = { _compiled_state = self._state_machine.finish_state }

		self._state_machine:compile()
	end
end

function states.StateMachine.method:dump_graph(file)
	file:write('digraph state_machine {\n')
	file:write('fail [fillcolor="#ee0000",style="filled"]\n')
	file:write('finish [fillcolor="#00ee00",style="filled"]\n')
	file:write(string.format('%s [fillcolor="#00d7ff",style="filled"]\n', self._initial._name))

	for name, state in pairs(self._states) do
		state:_dump_graph(file)
	end

	file:write("}\n")
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
	self.owner = owner
end

function states.StateMachineInstance.method:update(...)
	local current = self.states[self._instance.state]
	if not current then
		error("state machine instance has finished")
	end

	current._state:_update(self, ...)
end

function states.StateMachineInstance.method:trigger(name, ...)
	local current = self.states[self._instance.state]
	if not current then
		error("state machine instance has finished")
	end

	local trans = current[name]
	if not trans then
		error(string.format("no event named '%s' on state '%s'", name, current._name))
	end

	local newstate = trans(self.owner, ...)

	if newstate then
		self._instance:update(newstate)
	end
end

function states.StateMachineInstance.method:finish()
	self._instance:finish()
end


local state_machine_int = {}

local function state_machine_env(state_machine)
	state_machine_int.state_type = function (state_class)
		assert(class.isaclass(state_class, state.State), "state type must be of type State")
		state_machine._state_type = state_class
	end

	state_machine_int.initial = function (state)
		state_machine._initial = state
	end

	state_machine_int.state = function (...)
		assert(state_machine._state_type, "cannot create state without prior call to state_type()")
		return state_machine._state_type:new(...)
	end

	-- Fake special states
	state_machine_int.finish = state.State:new("finish")
	state_machine_int.fail = state.State:new("fail")

	-- Special default actions collection
	state_machine_int.any = state_machine._default

	-- Events proxying
	state_machine_int.events = {}
	state_machine_int.events.timeout = function(timeout)
		return { name = "timeouts", timeout = timeout }
	end
	setmetatable(state_machine_int.events, {
		__index = function (self, name)
			return { name = name }
		end,
		__newindex = function ()
			error("events is a read only table")
		end
	})

	return {
		__index = function (self, name)
			local ret

			-- Search in the state classes
			ret = state[name]
			if ret then return ret end

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
				value._name = key
				state_machine._states[key] = value
			else
				rawset(self, key, value)
			end
		end
	}
end

local state_machine = {}

state_machine.State = state.State
state_machine.BidirectionnalState = state.BidirectionnalState

function state_machine.new(name, def)
	assert(type(def) == 'function', "state machine definition must by a function")

	local sm = states.StateMachine:new(name)

	-- Add a metatable to the environment only during the definition
	-- of the grammar.
	local env = debug.getfenv(def)
	setmetatable(env, state_machine_env(sm))

	def()
	setmetatable(env, nil)

	sm:compile()

	if state_machine.debug then
		haka.log.warning("state_machine", "dumping '%s' state_machine graph to %s-state-machine.dot", sm.name, sm.name)
		f = io.open(string.format("%s-state-machine.dot", sm.name), "w+")
		sm:dump_graph(f)
		f:close()
	end

	return sm
end

state_machine.debug = false

haka.state_machine = state_machine

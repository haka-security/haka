
require('class')


local states = {}




states.State = class('State')

function states.State.method:__init(transitions)
	for n, f in pairs(transitions) do
		self[n] = f
	end
end

function states.State.method:merge(transitions)
	for k, f in pairs(transitions) do
		if not self[k] then
			self[k] = f
		end
	end
end


states.CompiledState = class('CompiledState')

function states.CompiledState.method:__init(state_machine, state, name)
	self._state = state_machine:create_state(name)
	self.name = name

	for n, f in pairs(state) do
		if n == 'timeouts' then
			assert(type(f) == 'table', "timeout transition must be a table")
			for timeout, callback in pairs(f) do
				assert(type(callback) == 'function', "transition must be a function")
				self._state:transition_timeout(timeout, callback)
			end
		else
			assert(type(f) == 'function', "transition must be a function")
			
			if n == 'error' then
				self._state:transition_error(f)
			elseif n == 'enter' then
				self._state:transition_enter(f)
			elseif n == 'leave' then
				self._state:transition_leave(f)
			elseif n == 'init' then
				self._state:transition_init(f)
			elseif n == 'finish' then
				self._state:transition_finish(f)
			else
				self[n] = f
				self['_internal_' .. n] = function (self, ...)
					local newstate = f(self, ...)
					if newstate then
						if isa(newstate, states.CompiledState) then
							return self._instance:update(newstate._state)
						else
							return self._instance:update(newstate)
						end
					end
				end
			end
		end
	end
end


states.StateMachine = class('StateMachine')

function states.StateMachine.method:__init(name)
	self._states = {}
	self.name = name
	self._default = {}
end

function states.StateMachine.method:state(transitions)
	assert(isa(self, states.StateMachine))
	return states.State:new(transitions)
end

function states.StateMachine.method:default(transitions)
	assert(isa(self, states.StateMachine))
	self._default = transitions
end

function states.StateMachine.method:compile()
	if not self._state_machine then
		local state_machine = haka.state_machine.state_machine(self.name)
		local state_table = {}
		local initial

		for name, state in pairs(self) do
			if name ~= 'initial' and isa(state, states.State) then
				state:merge(self._default)
				local new_state = states.CompiledState:new(state_machine, state, name)
				state_table[name] = new_state

				if state == self.initial then
					initial = new_state
				end
			end
		end

		if not initial then
			if isa(self.initial, states.State) then
				self.initial:merge(self._default)
				initial = states.CompiledState:new(state_machine, self.initial, 'initial')
				state_table['initial'] = initial
			else
				error("state machine must have in initial state")
			end
		end

		state_machine.initial = initial._state
		state_table.initial = initial

		state_table.ERROR = state_machine.error_state
		state_table.FINISH = state_machine.finish_state

		state_machine:compile()

		self._state_machine = state_machine
		self._state_table = state_table
	end
end

function states.StateMachine.method:instanciate()
	self:compile()
	return states.StateMachineInstance:new(self)
end


states.StateMachineInstance = class('StateMachineInstance')

function states.StateMachineInstance.property.get:state()
	local current = self.states[self._instance.state]
	return current
end

function states.StateMachineInstance.method:__init(state_machine)
	self._state_machine = state_machine
	self.states = state_machine._state_table
	self._instance = state_machine._state_machine:instanciate(self)
	self._instance:init()
end

function states.StateMachineInstance.method:__index(name)
	local current = rawget(self, 'states')[rawget(self, '_instance').state]
	local trans = current['_internal_' .. name]
	if not trans then
		error(string.format("no transition named '%s' on state '%s'", name, self.state.name))
	end
	return trans
end

function states.StateMachineInstance.method:finish()
	self._instance:finish()
end


function states.new(name)
	return states.StateMachine:new(name)
end

return states

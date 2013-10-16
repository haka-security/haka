
require('class')


local states = {}




states.State = class('State')

function states.State.method:__init(transitions)
	for n, f in pairs(transitions) do
		self[n] = f
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
			else
				self[n] = function (self, ...)
					haka.log.debug("state machine", "%s: %s transition on state '%s'", self._state_machine.name, n, name)
					local newstate = f(self, ...)
					if newstate then
						return self._instance:update(newstate._state)
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
	return states.State:new(transitions)
end

function states.StateMachine.method:default(transitions)
	self._default = transitions
end

function states.StateMachine.method:compile()
	if not self._state_machine then
		local state_machine = haka.state_machine.state_machine(self.name)
		local state_table = {}

		for name, state in pairs(self) do
			if name ~= "INITIAL" and isa(state, states.State) then
				for k, f in pairs(self._default) do
					if not state[k] then
						state[k] = f
					end
				end

				local new_state = states.CompiledState:new(state_machine, state, name)
				state_table[name] = new_state

				if state == self.INITIAL then
					state_machine.initial = new_state._state
					state_table.INITIAL = new_state
				end
			end
		end

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
end

function states.StateMachineInstance.method:__index(name)
	local trans = self.state[name]
	if not trans then
		error(string.format("no transition named '%s' on state '%s'", name, self.state.name))
	end
	return trans
end


function states.new(name)
	return states.StateMachine:new(name)
end

return states

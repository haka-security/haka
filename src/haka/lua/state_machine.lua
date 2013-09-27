
local states = {}


states.Transition = class()

function states.on_input(value)
	local transition = states.Transition:new()
	transition.type = 'INPUT'
	transition.input = value
	return transition
end

function states.on_timeout(timeout)
	local transition = states.Transition:new()
	transition.type = 'TIMEOUT'
	transition.timeout = timeout
	return transition
end

function states.on_error()
	local transition = states.Transition:new()
	transition.type = 'ERROR'
	return transition
end


states.StateMachine = class()

function states.StateMachine:__init()
	self._state_machine = haka.state_machine.state_machine()
end

function states.StateMachine:state(transitions)
	local state = self._state_machine:create_state()

	for t, f in pairs(transitions) do
		assert(isa(t, states.Transition), "key must be a transition")
		assert(type(f) == 'function', "value must be a function")
		
		if t.type == 'TIMEOUT' then
			state:transition_timeout(t.timeout, self, f)
		elseif t.type == 'ERROR' then
			state:transition_error(self, f)
		elseif t.type == 'INPUT' then
			state:transition_input(self, f)
		else
			error("invalid state transition type")
		end
	end

	return state
end

property(states.StateMachine, 'set').initial = function (self, initial)
	self._state_machine.initial = initial
end

property(states.StateMachine, 'get').initial = function (self)
	return self._state_machine.initial
end

function states.StateMachine:instanciate()
	return self._state_machine:instanciate()
end

function states.new()
	return states.StateMachine:new()
end

return states

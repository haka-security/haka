
local states = {}


states.State = class()

function states.State:__init()
	self.transitions = {}
end

function states.State:addtransition(reason, func)
	assert(isa(reason, states.Transition), "key must be a transition")
	assert(type(func) == 'function', "value must be a function")
	
	self.transitions[reason] = func
end

states.Transition = class()

function states.state(transitions)
	local state = states.State:new()

	for t, f in pairs(transitions) do
		state.addtransition(t, f)
	end

	return state
end


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
	self._states = {}
end

function states.StateMachine:state(transitions)
	local state = states.State:new()

	for t, f in pairs(transitions) do
		state:addtransition(t, f)
	end

	table.insert(self._states, state)
	return state
end

function states.StateMachine:instanciate()
	return { state = self.initial }
end

function states.new()
	return states.StateMachine:new()
end

haka.state_machine = states

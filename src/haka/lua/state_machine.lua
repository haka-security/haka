
local states = {}

states.Transition = class()

function states.Transition:__init(type)
	self.type = type
end

function states.on_input(value)
	local transition = states.Transition:new('INPUT')
	transition.input = value
	return transition
end

function states.on_timeout(timeout)
	local transition = states.Transition:new('TIMEOUT')
	transition.timeout = timeout
	return transition
end

states.on_error = states.Transition:new('ERROR')
states.on_enter = states.Transition:new('ENTER')
states.on_leave = states.Transition:new('LEAVE')


states.State = class()

function states.State:__init(transitions)
	self.transitions = transitions

	for t, f in pairs(transitions) do
		assert(isa(t, states.Transition), "key must be a transition")
		assert(type(f) == 'function', "value must be a function")
	end
end


states.StateMachine = class()

function states.StateMachine:__init(name)
	self._states = {}
	self._name = name
end

function states.StateMachine:state(transitions)
	return states.State:new(transitions)
end

function states.StateMachine:compile()
	if not self._state_machine then
		local state_machine = haka.state_machine.state_machine(self._name)
		local state_table = {}

		for name, state in pairs(self) do
			if name ~= "INITIAL" and isa(state, states.State) then
				local new_state = state_machine:create_state(name)

				for t, f in pairs(state.transitions) do
					if t.type == 'TIMEOUT' then
						new_state:transition_timeout(t.timeout, f)
					elseif t.type == 'ERROR' then
						new_state:transition_error(f)
					elseif t.type == 'INPUT' then
						new_state:transition_input(f)
					elseif t.type == 'ENTER' then
						new_state:transition_enter(f)
					elseif t.type == 'LEAVE' then
						new_state:transition_leave(f)
					else
						error("invalid state transition type")
					end
				end

				state_table[name] = new_state

				if state == self.INITIAL then
					state_machine.initial = new_state
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

function states.StateMachine:instanciate(context)
	self:compile()
	return self._state_machine:instanciate(self._state_table, context)
end

function states.new(name)
	return states.StateMachine:new(name)
end

return states

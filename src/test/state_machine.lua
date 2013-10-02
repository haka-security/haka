--[[
This test check the state machine transitions.
]]

require('protocol/ipv4')

local machine = haka.state_machine.new("test")

machine.state1 = machine:state {
	[haka.state_machine.on_enter()] = function (states, context)
		context.state = "state1"
		context.count = 0
	end,
	[haka.state_machine.on_input()] = function (states, context)
		return states.state2
	end,
	[haka.state_machine.on_timeout(0.5)] = function (states, context)
		context.count = context.count + 1
		if context.count > 5 then
			return states.state3
		end
	end
}

machine.state2 = machine:state {
	[haka.state_machine.on_enter()] = function (states, context)
		return states.ERROR
	end,
	[haka.state_machine.on_error()] = function (states, context)
		return states.FINISH
	end
}

machine.state3 = machine:state {
	[haka.state_machine.on_input()] = function (states, context)
		return states.state1
	end,
	[haka.state_machine.on_timeout(1)] = function (states, context)
		return states.state1
	end,
	[haka.state_machine.on_leave()] = function (states, context)
		context.state = nil
		return states.state2
	end,
}

machine.INITIAL = machine.state1

local context = {}
local instance = machine:instanciate(context)

-- Sleep utility function
local clock = os.clock
function sleep(n)  -- seconds
	local t0 = clock()
	while clock() - t0 <= n do end
end

haka.rule {
	hooks = { 'ipv4-up' },
	eval = function (self, pkt)
		sleep(0.2)
	end
}

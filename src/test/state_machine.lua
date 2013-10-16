--[[
This test check the state machine transitions.
]]

require('protocol/ipv4')

local machine = haka.state_machine.new("test")

machine.state1 = machine:state {
	enter = function (self)
		self.mystate = "state1"
		self.count = 0
	end,
	update = function (self, direction, input)
		print(direction, "received:", input)
		return self.states.state4
	end,
	timeouts = {
		[0.5] = function (self)
			self.count = self.count + 1
			if self.count > 5 then
				return self.states.state3
			end
		end
	}
}

machine.state2 = machine:state {
	enter = function (self)
		return self.states.ERROR
	end,
	error = function (self)
		return self.states.FINISH
	end
}

machine.state3 = machine:state {
	update = function (self)
		return self.states.state1
	end,
	timeouts = {
		[1] = function (self)
			return self.states.state1
		end,
	},
	leave = function (self)
		self.mystate = nil
		return self.states.state2
	end,
}

machine.state4 = machine:state {
	update = function (self, direction, output)
		print(direction, "received:", output)
		return self.states.state1
	end
}

machine.INITIAL = machine.state1

local context = {}

haka.rule {
	hook = haka.event('ipv4', 'receive_packet'),
	eval = function (self, pkt)
		if not context.instance then
			context.instance = machine:instanciate()
			context.instance.hello = "hello"
		end

		if not context.input then
			context.instance:update('input', string.format("hello from state '%s'", context.instance.state.name))
			context.input = true
		elseif not context.output then
			context.instance:update('output', string.format("hello from state '%s'", context.instance.state.name))
			context.output = true
		else
			local count = 0
			while context.instance.state do
				-- busy wait
				count = count+1
			end
		end
	end
}

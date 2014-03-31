-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- This test check the state machine transitions.

require('protocol/ipv4')

local machine = haka.state_machine("test")

machine:default {
	error = function (self)
		print("error on", self.state.name)
		return self.states.FINISH
	end,
	finish = function (self)
		print("finished", self.hello)
	end,
	enter = function (self)
		print("enter", self.state.name)
	end,
	leave = function (self)
		print("leave", self.state.name)
	end,
}

machine.state1 = machine:state {
	init = function (self)
		print("initialize state1")
	end,
	enter = function (self)
		self.mystate = "state1"
		self.count = 0
		print("enter state1")
	end,
	update = function (self, direction, input)
		print(direction, "state1 received:", input)
		return self.states.state4
	end,
	timeouts = {
		[0.5] = function (self)
			print("timeout from state1")
			self.count = self.count + 1
			if self.count > 5 then
				return self.states.state3
			end
		end
	}
}

machine.state2 = machine:state {
	enter = function (self)
		print("enter state2")
		return self.states.ERROR
	end
}

machine.state3 = machine:state {
	update = function (self)
		return self.states.state1
	end,
	timeouts = {
		[1] = function (self)
			print("timeout from state3")
			print("go to state1")
			return self.states.state1
		end,
	},
	leave = function (self)
		print("leave state3")
		self.mystate = nil
		return self.states.state2
	end,
}

machine.state4 = machine:state {
	update = function (self, direction, output)
		print(direction, "state4 received:", output)
		return self.states.state1
	end
}

machine.initial = machine.state1

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

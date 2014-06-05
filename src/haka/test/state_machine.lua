-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- This test check the state machine transitions.

local ipv4 = require('protocol/ipv4')

local machine = haka.state_machine("test", function ()
	default_transitions{
		error = function (self)
			print("error on", self.instance.current)
		end,
		finish = function (self)
			print("finished", self.hello)
		end,
		enter = function (self)
			print("enter", self.instance.current)
		end,
		leave = function (self)
			print("leave", self.instance.current)
		end,
	}

	state1 = state {
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
			return 'state4'
		end,
		timeouts = {
			[0.5] = function (self)
				print("timeout from state1")
				self.count = self.count + 1
				if self.count > 5 then
					return 'state3'
				end
			end
		}
	}

	state2 = state {
		enter = function (self)
			print("enter state2")
			return 'error'
		end
	}

	state3 = state {
		update = function (self)
			return 'state1'
		end,
		timeouts = {
			[1] = function (self)
				print("timeout from state3")
				print("go to state1")
				return 'state1'
			end,
		},
		leave = function (self)
			print("leave state3")
			self.mystate = nil
			return 'state2'
		end,
	}

	state4 = state {
		update = function (self, direction, output)
			print(direction, "state4 received:", output)
			return 'state1'
		end
	}

	initial(state1)
end)

local context = {}

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (self, pkt)
		if not context.instance then
			context.hello = "hello"
			context.cnt = 0
			context.instance = machine:instanciate(context)
		end

		if context.cnt < 2 then
			context.instance:transition('update', context.cnt, string.format("hello from state '%s'", context.instance.current))
			context.cnt = context.cnt+1
		end
	end
}

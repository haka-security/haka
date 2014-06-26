-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

-- This test check the state machine transitions.

local class = require('class')
local ipv4 = require('protocol/ipv4')

local TestState = class.class('TestState', haka.state_machine.State)

function TestState.method:__init()
	class.super(TestState).__init(self)
	table.merge(self._actions, {
		test = {},
	});
end


local machine = haka.state_machine.new("test", function ()
	state_type(TestState)

	state1 = state()
	state2 = state()
	state3 = state()
	state4 = state()

	any:on{
		event = events.fail,
		execute = function (self)
			print("fail on", self.instance.current)
		end,
	}

	any:on{
		event = events.finish,
		execute = function (self)
			print("finished", self.hello)
		end,
	}

	any:on{
		event = events.enter,
		execute = function (self)
			if self.instance then
				print("enter", self.instance.current)
			else
				print("enter initial state")
			end
		end,
	}

	any:on{
		event = events.leave,
		execute = function (self)
			print("leave", self.instance.current)
		end,
	}

	state1:on{
		event = events.init,
		execute = function (self)
			print("initialize state1")
		end,
	}

	state1:on{
		event = events.enter,
		execute = function (self)
			self.mystate = "state1"
			self.count = 0
			print("enter state1")
		end,
	}

	state1:on{
		event = events.test,
		execute = function (self, direction, input)
			print(direction, "state1 received:", input)
		end,
		jump = state4,
	}

	state1:on{
		event = events.timeout(0.5),
		when = function (self)
			print("timeout from state1")
			self.count = self.count + 1
			return self.count > 5
		end,
		jump = state3,
	}

	state2:on{
		event = events.enter,
		execute = function (self)
			print("enter state2")
		end,
		jump = fail,
	}

	state3:on{
		event = events.test,
		jump = state1,
	}

	state3:on{
		event = events.timeout(1),
		execute = function (self)
			print("timeout from state3")
			print("go to state1")
		end,
		jump = state1,
	}

	state3:on{
		event = events.leave,
		execute = function (self)
			print("leave state3")
			self.mystate = nil
		end,
		jump = state2,
	}

	state4:on{
		event = events.test,
		execute = function (self, direction, output)
			print(direction, "state4 received:", output)
		end,
		jump = state1,
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
			context.instance:trigger('test', context.cnt, string.format("hello from state '%s'", context.instance.current))
			context.cnt = context.cnt+1
		end
	end
}

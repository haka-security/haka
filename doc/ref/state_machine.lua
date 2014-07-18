-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local my_state_machine = haka.state_machine("test", function ()
	state_type{
		events = { 'test' },
		update = function (self, state_machine)
			state_machine:trigger('test')
		end
	}

	foo = state()
	bar = state()

	foo:on{
		event = events.test,
		execute  = function (self)
			print("update")
		end,
		jump = bar -- jump to the state bar
	}

	bar:on{
		event = events.enter,
		execute  = function (self)
			print("finish")
		end
	}

	initial(foo) -- start on state foo
end)

local context = {}
local instance = my_state_machine:instanciate(context)

instance:update() -- trigger the event test

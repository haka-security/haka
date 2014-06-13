-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestStateMachine = {}

function TestStateMachine:test_state_machine_compile()
	-- Given
	-- nothing
	-- When
	local sm = haka.state_machine("smtest", function()
		my_foo_state = state.basic()
		my_bar_state = state.basic()

		my_foo_state:on{
			event = events.enter,
			jump = my_bar_state,
		}

		initial(my_foo_state)
	end)

	-- Then
	assert(sm)
end

function TestStateMachine:test_state_machine_run()
	-- Given
	local ctx = {
		foo = false,
		bar = false,
	}
	local sm = haka.state_machine("smtest", function()
		my_foo_state = state.basic({ events.test })
		my_bar_state = state.basic({ events.test })

		my_foo_state:on{
			event = events.test,
			jump = my_bar_state,
			action = function(self)
				self.foo = true
			end,
		}

		my_bar_state:on{
			event = events.test,
			jump = finish,
			action = function(self)
				self.bar = true
			end,
		}

		initial(my_foo_state)
	end):instanciate(ctx)

	-- When
	sm:update("test")
	sm:update("test")

	-- Then
	assertTrue(ctx.foo)
	assertTrue(ctx.bar)
	assertTrue(sm._instance.finished)
end

function TestStateMachine:test_state_machine_fail()
	-- Given
	local ctx = {
		test = false
	}
	local sm = haka.state_machine("smtest", function()
		my_foo_state = state.basic({ events.test })

		my_foo_state:on{
			event = events.test,
			action = function(self)
				self.test = true
			end,
			jump = fail,
		}

		initial(my_foo_state)
	end):instanciate(ctx)

	-- When
	sm:update("test")

	-- Then
	assertTrue(ctx.test)
	assertTrue(sm._instance.failed)
end

function TestStateMachine:test_state_machine_defaults()
	-- Given
	local ctx = {
		test = false
	}
	local sm = haka.state_machine("smtest", function()
		my_foo_state = state.basic({ events.test })

		any:on{
			event = events.test,
			action = function(self)
				self.test = true
			end
		}

		initial(my_foo_state)
	end):instanciate(ctx)

	-- When
	sm:update("test")

	-- Then
	assertTrue(ctx.test)
end

function TestStateMachine:test_state_machine_bidirectionnal_run()
	-- Given
	local ctx = {
		up = false,
		down = false,
	}
	local upbuf = haka.vbuffer_from("up")
	local downbuf = haka.vbuffer_from("down")
	local g = haka.grammar.new("bidirectionnal", function()
		up = field("up", token("up")
			:const("up"))

		down = field("down", token("down")
			:const("down"))

		export(up, down)
	end)
	local state_machine = haka.state_machine("smtest", function()
		my_up_state = state.bidirectionnal(g.up, g.down)
		my_down_state = state.bidirectionnal(g.up, g.down)

		my_up_state:on{
			event = events.up,
			jump = my_down_state,
			action = function(self)
				self.up = true
			end,
		}

		my_up_state:on{
			event = events.down,
			jump = fail
		}

		my_down_state:on{
			event = events.down,
			jump = finish,
			action = function(self)
				self.down = true
			end,
		}

		my_down_state:on{
			event = events.up,
			jump = fail
		}

		initial(my_up_state)
	end):instanciate(ctx)

	-- When
	state_machine:update(upbuf, "up", nil)
	state_machine:update(downbuf, "down", nil)

	-- Then
	assertTrue(state_machine._instance.finished)
	assertTrue(ctx.up)
	assertTrue(ctx.down)
end

function TestStateMachine:test_state_machine_bidirectionnal_fail()
	-- Given
	local ctx = {
		up = false,
		down = false,
	}
	local upbuf = haka.vbuffer_from("up")
	local downbuf = haka.vbuffer_from("down")
	local g = haka.grammar.new("bidirectionnal", function()
		up = field("up", token("up")
			:const("up"))

		down = field("down", token("down")
			:const("down"))

		export(up, down)
	end)
	local state_machine = haka.state_machine("smtest", function()
		my_up_state = state.bidirectionnal(g.up, g.down)
		my_down_state = state.bidirectionnal(g.up, g.down)

		my_up_state:on{
			event = events.up,
			jump = my_down_state,
			action = function(self)
				self.up = true
			end,
		}

		my_up_state:on{
			event = events.down,
			jump = fail
		}

		my_down_state:on{
			event = events.down,
			jump = finish,
			action = function(self)
				self.down = true
			end,
		}

		my_down_state:on{
			event = events.up,
			jump = fail
		}

		initial(my_up_state)
	end):instanciate(ctx)

	state_machine:update(upbuf, "up", nil)

	-- When
	state_machine:update(upbuf, "up", nil)

	-- Then
	assertTrue(state_machine._instance.failed)
	assertTrue(ctx.up)
	assertFalse(ctx.down)
end

function TestStateMachine:test_state_machine_bidirectionnal_parse_fail()
	-- Given
	local ctx = {
		up = false,
		down = false,
		parse_error = false,
	}
	local upbuf = haka.vbuffer_from("up")
	local downbuf = haka.vbuffer_from("wrong")
	local g = haka.grammar.new("bidirectionnal", function()
		up = field("up", token("up")
			:const("up"))

		down = field("down", token("down")
			:const("down"))

		export(up, down)
	end)
	local state_machine = haka.state_machine("smtest", function()
		my_up_state = state.bidirectionnal(g.up, g.down)
		my_down_state = state.bidirectionnal(g.up, g.down)

		any:on{
			event = events.parse_error,
			action = function(self)
				self.parse_error = true
			end,
			jump = fail
		}

		my_up_state:on{
			event = events.up,
			jump = my_down_state,
			action = function(self)
				self.up = true
			end,
		}

		my_down_state:on{
			event = events.down,
			jump = finish,
			action = function(self)
				self.down = true
			end,
		}

		initial(my_up_state)
	end):instanciate(ctx)

	state_machine:update(upbuf, "up", nil)

	-- When
	state_machine:update(downbuf, "down", nil)

	-- Then
	assertTrue(state_machine._instance.failed)
	assertTrue(ctx.up)
	assertFalse(ctx.down)
	assertTrue(ctx.parse_error)
end

LuaUnit:setVerbosity(2)
assert(LuaUnit:run('TestStateMachine') == 0)

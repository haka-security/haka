-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestStateMachine = {}

function TestStateMachine:test_state_machine_compile()
	-- Given
	-- nothing
	-- When
	local state_machine = haka.state_machine("smtest", function()
		my_foo_state = state.basic()
		my_bar_state = state.basic()

		my_foo_state:on{
			event = events.enter,
			jump = my_bar_state,
		}

		initial(my_foo_state)
	end)

	-- Then
	assert(state_machine)
end

function TestStateMachine:test_state_machine_run()
	-- Given
	local test_foo = false
	local test_bar = false
	local foobuf = haka.vbuffer_from("foo")
	local barbuf = haka.vbuffer_from("bar")
	local g = haka.grammar.new("foobar", function()
		foo = field("foo", token("foo")
			:const("foo"))

		bar = field("bar", token("bar")
			:const("bar"))

		export(foo, bar)
	end)
	local state_machine = haka.state_machine("smtest", function()
		my_foo_state = state.grammar(g.foo)
		my_bar_state = state.grammar(g.bar)

		my_foo_state:on{
			event = events.up,
			jump = my_bar_state,
			action = function ()
				test_foo = true
			end,
		}

		my_bar_state:on{
			event = events.up,
			jump = finish,
			action = function ()
				test_bar = true
			end,
		}

		initial(my_foo_state)
	end)

	-- When
	local sm_instance = state_machine:instanciate(self)
	sm_instance:update(foobuf, "up", nil)
	sm_instance:update(barbuf, "up", nil)

	-- Then
	assertTrue(sm_instance._instance.finished)
	assertTrue(test_foo)
	assertTrue(test_bar)
end

function TestStateMachine:test_state_machine_fail()
	-- Given
	local foobuf = haka.vbuffer_from("foo")
	local g = haka.grammar.new("foobar", function()
		foo = field("foo", token("foo")
			:const("foo"))

		export(foo)
	end)
	local state_machine = haka.state_machine("smtest", function()
		my_foo_state = state.grammar(g.foo)

		my_foo_state:on{
			event = events.up,
			jump = fail,
		}

		initial(my_foo_state)
	end)

	-- When
	local sm_instance = state_machine:instanciate(self)
	sm_instance:update(foobuf, "up", nil)

	-- Then
	assertTrue(sm_instance._instance.failed)
end

function TestStateMachine:test_state_machine_defaults()
	-- Given
	local test = false
	local sm = haka.state_machine("smtest", function()
		my_foo_state = state.basic()

		any:on{
			event = events.up,
			action = function ()
				test = true
			end
		}

		initial(my_foo_state)
	end):instanciate(self)

	-- When
	sm:update("up")

	-- Then
	assertTrue(test)
end



LuaUnit:setVerbosity(2)
assert(LuaUnit:run('TestStateMachine') == 0)

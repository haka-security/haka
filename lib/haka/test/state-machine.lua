-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestStateMachine = {}

function TestStateMachine:test_state_machine_compile()
	-- Given
	local buf = haka.vbuffer_from("foobar")
	local g = haka.grammar.new("foobar", function()
		foo = field("foo", token("foo")
			:const("foo"))

		bar = field("bar", token("bar")
			:const("bar"))

		export(foo, bar)
	end)

	-- When
	local state_machine = haka.state_machine("tcp", function()
		my_foo_state = state.basic(g.foo)
		my_bar_state = state.basic(g.bar)

		my_foo_state:on{
			event = events.up,
			check = function (self, pkt) return pkt.flags.syn and pkt.flags.ack end,
			jump = my_bar_state,
			action = function (self, pkt)
				self.stream[self.output]:init(pkt.seq+1)
				pkt:send()
			end
		}

		initial(my_foo_state)
	end)

	-- Then
	assert(state_machine)
end

function TestStateMachine:test_state_machine_run()
	-- Given
	local foobuf = haka.vbuffer_from("foo")
	local barbuf = haka.vbuffer_from("bar")
	local g = haka.grammar.new("foobar", function()
		foo = field("foo", token("foo")
			:const("foo"))

		bar = field("bar", token("bar")
			:const("bar"))

		export(foo, bar)
	end)
	local state_machine = haka.state_machine("tcp", function()
		my_foo_state = state.basic(g.foo)
		my_bar_state = state.basic(g.bar)

		my_foo_state:on{
			event = events.up,
			jump = my_bar_state,
			action = function ()
				debug.pprint("woot woot we are in foo state action")
			end,
		}

		my_bar_state:on{
			event = events.up,
			jump = finish,
			action = function ()
				debug.pprint("woot woot we are in bar state action")
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
end

function TestStateMachine:test_state_machine_fail()
	-- Given
	local foobuf = haka.vbuffer_from("foo")
	local g = haka.grammar.new("foobar", function()
		foo = field("foo", token("foo")
			:const("foo"))

		export(foo)
	end)
	local state_machine = haka.state_machine("tcp", function()
		my_foo_state = state.basic(g.foo)

		my_foo_state:on{
			event = events.up,
			jump = fail,
			action = function ()
				debug.pprint("woot woot we are in foo state action")
			end,
		}

		initial(my_foo_state)
	end)

	-- When
	local sm_instance = state_machine:instanciate(self)
	sm_instance:update(foobuf, "up", nil)

	-- Then
	assertTrue(sm_instance._instance.failed)
end

LuaUnit:setVerbosity(2)
assert(LuaUnit:run('TestStateMachine') == 0)

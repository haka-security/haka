-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestGrammarTry = {}

function TestGrammarTry:test_try_compile()
	-- Given
	--   nothing
	-- When
	local gr = haka.grammar.new("test", function ()
		elem = try{
			number(1)
		}

		export(elem)
	end)
	-- Then
	assertTrue(gr.elem)
end

function TestGrammarTry:test_try_catch_error()
	-- Given
	local vbuf = haka.vbuffer_from("bar")
	local gr = haka.grammar.new("test", function ()
		elem = try{
			field("foo", token("foo")),
			field("bar", token("bar")),
		}

		export(elem)
	end)
	-- When
	local res = gr.elem:parse(vbuf:pos("begin"))
	-- Then
	assertEquals(res.bar, "bar")
end

function TestGrammarTry:test_try_dual()
	-- Given
	local vbuf = haka.vbuffer_from("bar")
	local gr = haka.grammar.new("test", function ()
		elem = try{
			try{
				field("fee", token("fee")),
				field("foo", token("foo")),
			},
			field("bar", token("bar"))
		}
		export(elem)
	end)
	-- When
	local res = gr.elem:parse(vbuf:pos("begin"))
	-- Then
	assertEquals(res.bar, "bar")
end

function TestGrammarTry:test_try_does_not_leave_result_on_fail()
	-- Given
	local vbuf = haka.vbuffer_from("bar foo")
	local gr = haka.grammar.new("test", function ()
		elem = try{
			field("rec_fee", record{
				field("bar", token("bar")),
				token(" "),
				field("fee", token("fee")),
			}),
			field("rec_foo", record{
				field("bar", token("bar")),
				token(" "),
				field("foo", token("foo")),
			}),
		}

		export(elem)
	end)
	-- When
	local res = gr.elem:parse(vbuf:pos("begin"))
	-- Then
	assertIsNil(res.rec_fee)
	assertIsTable(res.rec_foo)
	assertEquals(res.rec_foo.bar, "bar")
	assertEquals(res.rec_foo.foo, "foo")
end

addTestSuite('TestGrammarTry')

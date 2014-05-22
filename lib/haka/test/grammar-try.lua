-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')


TestGrammarTry = {}

function TestGrammarTry:gen_stream(f)
	local data = { "bar fo", "o dea", "d beef", " cof", "fee", " foo foo " }
	local stream = haka.vbuffer_stream()
	local manager = haka.vbuffer_stream_comanager:new(stream)
	manager:start(0, f)

	for i, d in ipairs(data) do
		local current = stream:push(haka.vbuffer_from(d))
		if i == #data then
			stream:finish()
		end

		manager:process_all(current)

		while stream:pop() do end
	end
end

function TestGrammarTry:test_compile()
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

function TestGrammarTry:test_catch_error()
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
	assertTrue(res.bar)
	assertIsNil(res.foo)
end

LuaUnit:setVerbosity(1)
assert(LuaUnit:run('TestGrammarTry') == 0)

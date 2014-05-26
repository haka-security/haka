-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestGrammarToken = {}

function TestGrammarToken:test_token()
	-- Given
	local buf = haka.vbuffer_from("abcdefghijklmnopqrstuvwxyz0123456789")
	local grammar = haka.grammar.new("test", function ()
		elem = field("token", token("[a-z]*"))

		export(elem)
	end)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	assertEquals(result.token, "abcdefghijklmnopqrstuvwxyz")
end

function TestGrammarToken:test_raw_token()
	-- Given
	local buf = haka.vbuffer_from("abcdefghijklmnopqrstuvwxyz0123456789")
	local grammar = haka.grammar.new("test", function ()
		elem = field("token", raw_token("[a-z]*"))

		export(elem)
	end)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	assertEquals(result.token:asstring(), "abcdefghijklmnopqrstuvwxyz")
end

LuaUnit:setVerbosity(2)
assert(LuaUnit:run('TestGrammarToken') == 0)

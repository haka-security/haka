-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestGrammar = {}

function TestGrammar:test_union()
	-- Given
	local buf = haka.vbuffer_from("\xC1")
	local grammar = haka.grammar.union{
		haka.grammar.field('a', haka.grammar.number(2)),
		haka.grammar.field('b', haka.grammar.number(8)),
	}:compile()

	-- When
	local result = grammar:parse(buf:pos('begin'))

	assertEquals(result.a, 3)
	assertEquals(result.b, 193)
end

LuaUnit:setVerbosity(2)
assert(LuaUnit:run('TestGrammar') == 0)

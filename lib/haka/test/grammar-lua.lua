-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestGrammar = {}

function TestGrammar:test_union()
	-- Given
	local buf = haka.vbuffer_from("\xC1")
	local grammar = haka.grammar.new("test", function ()
		elem = union{
			field('a', number(2)),
			field('b', number(8))
		}

		export(elem)
	end)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	assertEquals(result.a, 3)
	assertEquals(result.b, 193)
end

LuaUnit:setVerbosity(2)
assert(LuaUnit:run('TestGrammar') == 0)

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestGrammarUnion = {}

function TestGrammarUnion:test_union()
	-- Given
	local buf = haka.vbuffer_from("\x42\xC1\xAA")
	local grammar = haka.grammar.new("test", function ()
		elem = record{
			field('a', number(7)),
			union{
				field('b', number(8)),
				field('c', number(3))
			},
			field('d', number(9))
		}

		export(elem)
	end)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	assertEquals(result.a, 0x21)
	assertEquals(result.b, 96)
	assertEquals(result.c, 3)
	assertEquals(result.d, 0x1AA)
end

addTestSuite('TestGrammarUnion')

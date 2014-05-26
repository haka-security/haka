-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')
local class = require('class')

TestGrammarInheritance = {}

function TestGrammarInheritance:test_inheritance_compile()
	-- Given
	local human = haka.grammar.new("human", function ()
		head = token('head')

		body = record{
			field('shoulders', token('shoulders')),
			field('arm', token('arm')),
		}

		export(head, body)
	end)

	-- When
	local superman = haka.grammar.new("superman", function()
		extend(human)

		body = field('tentacle', token('tentacle'))
	end)

	-- Then
	assertTrue(superman.body)
	assertEquals(class.classof(superman.body).name, "DGToken")
end

function TestGrammarInheritance:test_inheritance_override()
	-- Given
	local human = haka.grammar.new("human", function ()
		finger = token("finger")

		hand = sequence{
			finger,
		}

		export(hand)
	end)

	-- When
	local superman = haka.grammar.new("superman", function()
		extend(human)

		finger = bytes(28)
	end)

	-- Then
	assertEquals(class.classof(superman.hand._next).name, "DGBytes")
end

LuaUnit:setVerbosity(2)
assert(LuaUnit:run('TestGrammarInheritance') == 0)

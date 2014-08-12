-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
	assertEquals(class.classof(superman.hand._next._next).name, "DGBytes")
end

function TestGrammarInheritance:test_proxy()
	-- Given
	local human = haka.grammar.new("human", function ()
		finger = token("finger")

		hand = record{
			field("finger", finger),
		}

		export(hand)
	end)
	-- Then
	assertEquals(human.hand._next._next._next.name, "finger")
end

function TestGrammarInheritance:test_proxy_error()
	-- Given
	assertError("undefined entity: finger", function ()
		local human = haka.grammar.new("human", function ()
			define("finger")
	
			hand = record{
				field("finger", finger),
			}
	
			export(hand)
		end)
	end)
end

function TestGrammarInheritance:test_inheritance_multiple()
	-- Given
	local human = haka.grammar.new("human", function ()
		finger = token("finger")

		hand = sequence{
			finger,

		}

		export(hand)
	end)

	-- When
	local superman = haka.grammar.new("superman", function ()
		extend(human)

		finger = empty()
	end)

	local superwoman = haka.grammar.new("superwoman", function ()
		extend(superman)
	
		finger = bytes(32)
	end)

	-- Then
	assertEquals(class.classof(superwoman.hand._next._next).name, "DGBytes")
end

function TestGrammarInheritance:test_recursion()
	-- Given
	local buf = haka.vbuffer_from("\x02\x01\x00\x00")
	local g = haka.grammar.new("recurs", function ()
		define("root")

		root = record{
			field("count", number(8)),
			field("child", array(root)
				:count(function (self, ctx)
					return ctx:result(-2).count
				end)
			),
		}

		export(root)
	end)
	-- When
	local ret = g.root:parse(buf:pos('begin'))
	-- Then
	assertEquals(ret.count, 2)
	assertEquals(ret.child[1].count, 1)
	assertEquals(ret.child[1].child[1].count, 0)
	assertEquals(ret.child[2].count, 0)
end

function TestGrammarInheritance:test_export_unknown()
	-- Given
	assertError("can only export named rules", function ()
		local gr = haka.grammar.new("grammar", function ()
			head = token('head')

			export(head, body)
		end)
	end)
end

addTestSuite('TestGrammarInheritance')

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestGrammarCompilation = {}

function TestGrammarCompilation:setUp(test_name)
	os.remove(test_name.."_grammar.so")
end

function TestGrammarCompilation:test_new_grammar_compile(fname)
	-- Given
	os.remove(fname.."_grammar.c")
	local grammar = function ()
		elem = record{
			field("num", number(8)),
		}:apply(function (value)
			value.ok = true
		end)

		export(elem)
	end

	-- When
	local grammar = haka.grammar.new(fname, grammar, true)

	-- Then
	assertNotEquals(grammar, nil)
end

function TestGrammarCompilation:test_new_grammar_create_multiple_parsers(fname)
	-- Given
	local grammar = function ()
		foo = record{
			field("num", number(8)),
		}

		bar = record{
			field("num", number(8)),
		}

		export(foo)
		export(bar)
	end

	-- When
	local grammar = haka.grammar.new(fname, grammar, true)

	-- Then
	assert(grammar.foo)
	assert(grammar.bar)
end

function TestGrammarCompilation:test_apply_on_record(fname)
	-- Given
	local buf = haka.vbuffer_from("\x42")
	local grammar = haka.grammar.new(fname, function ()
		elem = record{
			field("num", number(8)),
		}
		export(elem)
	end, true)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	-- Then
	assertEquals(result.num, 0x42)
end

function TestGrammarCompilation:test_apply_on_sequence(fname)
	-- Given
	local buf = haka.vbuffer_from("\x42\x43")
	local grammar = haka.grammar.new(fname, function ()
		elem = sequence{
			number(8),
			record{
				field("num", number(8)),
			},
		}
		export(elem)
	end, true)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	-- Then
	assertEquals(result.num, 0x43)
end

function TestGrammarCompilation:test_apply_on_union(fname)
	-- Given
	local buf = haka.vbuffer_from("\x42")
	local grammar = haka.grammar.new(fname, function ()
		elem = union{
			field("foo", number(4)),
			field("bar", number(8)),
		}
		export(elem)
	end, true)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	-- Then
	assertEquals(result.foo, 0x4)
	assertEquals(result.bar, 0x42)
end

function TestGrammarCompilation:test_apply_on_branch(fname)
	-- Given
	local buf = haka.vbuffer_from("\x42")
	local grammar = haka.grammar.new(fname, function ()
		my_branch = branch({
			foo = field("num", number(8)),
			bar = field("num", number(4)),
		}, function(result, context)
			return "bar"
		end)

		export(my_branch)
	end, true)

	-- When
	local result = grammar.my_branch:parse(buf:pos('begin'))

	-- Then
	assertEquals(result.num, 0x4)
end

function TestGrammarCompilation:test_apply_on_branch_with_default(fname)
	-- Given
	local buf = haka.vbuffer_from("\x42")
	local grammar = haka.grammar.new(fname, function ()
		my_branch = branch({
			foo = field("num", number(8)),
			default = field("num", number(4)),
		}, function(result, context)
			return "bar"
		end)

		export(my_branch)
	end, true)

	-- When
	local result = grammar.my_branch:parse(buf:pos('begin'))

	-- Then
	assertEquals(result.num, 0x4)
end

function TestGrammarCompilation:test_apply_on_branch_with_multiple_elements(fname)
	-- Given
	local buf = haka.vbuffer_from("\x42")
	local grammar = haka.grammar.new(fname, function ()
		my_branch = branch({
			foo = field("num", number(8)),
			bar = record{
				field("first", number(4)),
				field("second", number(4)),
			}
		}, function(result, context)
			return "bar"
		end)

		export(my_branch)
	end, true)

	-- When
	local result = grammar.my_branch:parse(buf:pos('begin'))

	-- Then
	assertEquals(result.first, 0x4)
	assertEquals(result.second, 0x2)
end

function TestGrammarCompilation:test_apply_on_recurs(fname)
	-- Given
	local buf = haka.vbuffer_from("\x02\x01\x00\x00")
	local done = false
	local grammar = haka.grammar.new(fname, function ()
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
	end, true)

	-- When
	local ret = grammar.root:parse(buf:pos('begin'))

	-- Then
	assertEquals(ret.count, 2)
	assertEquals(ret.child[1].count, 1)
	assertEquals(ret.child[1].child[1].count, 0)
	assertEquals(ret.child[2].count, 0)
end

function TestGrammarCompilation:test_apply_on_token(fname)
	-- Given
	local buf = haka.vbuffer_from("GET toto")
	local done = false
	local grammar = haka.grammar.new(fname, function ()
		elem = record{
			field("foo", token('[^()<>@,;:%\\"/%[%]?={}[:blank:]]+')),
			token('[[:blank:]]+'),
			field("bar", token('toto')),
		}

		export(elem)
	end, true)

	-- When
	local ret = grammar.elem:parse(buf:pos('begin'))

	-- Then
	assertEquals(ret.foo, "GET")
	assertEquals(ret.bar, "toto")
end

addTestSuite('TestGrammarCompilation')

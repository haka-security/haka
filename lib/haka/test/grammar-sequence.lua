-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestGrammarSequence = {}

function TestGrammarSequence:test_sequence_data_release()
	--Given
	local grammar = haka.grammar.new("test", function ()
		elem = sequence{
			token('first'),
			token('second')
		}
		export(elem)
	end)

	local stream = haka.vbuffer_stream()
	local manager = haka.vbuffer_stream_comanager:new(stream)

	manager:start(0, function (iter)
		local result = grammar.elem:parse(iter)
	end)

	local current = stream:push(haka.vbuffer_from("first"))
	manager:process_all(current)
	assertEquals(stream:pop():sub():asstring(), "first")

	current = stream:push(haka.vbuffer_from("second"))
	stream:finish()
	manager:process_all(current)
	assertEquals(stream:pop():sub():asstring(), "second")
end

function TestGrammarSequence:test_sequence_data_retain()
	--Given
	local grammar = haka.grammar.new("test", function ()
		elem = record{
			token('first'),
			token('second')
		}
		export(elem)
	end)

	local stream = haka.vbuffer_stream()
	local manager = haka.vbuffer_stream_comanager:new(stream)

	manager:start(0, function (iter)
		local result = grammar.elem:parse(iter)
	end)

	local current = stream:push(haka.vbuffer_from("first"))
	manager:process_all(current)
	assertEquals(stream:pop(), nil)

	current = stream:push(haka.vbuffer_from("second"))
	stream:finish()
	manager:process_all(current)
	assertNotEquals(stream:pop():sub():asstring(), nil)
end

addTestSuite('TestGrammarSequence')

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestGrammarBytes = {}

function TestGrammarBytes:gen_stream(data, f)
	local stream = haka.vbuffer_stream()
	local manager = haka.vbuffer_stream_comanager:new(stream)
	manager:start(0, f)

	for i, d in ipairs(data) do
		local current = stream:push(haka.vbuffer_from(d))
		if i == #data then
			stream:finish()
		end

		manager:process_all(current)
	end
end

function TestGrammarBytes:test_byte_until()
	-- Given
	local buf = haka.vbuffer_from("foobar")
	local grammar = haka.grammar.new("test", function ()
		elem = record{
			field("foo", bytes():untiltoken("bar")),
			field("bar", bytes()),
		}
		export(elem)
	end)

	-- When
	local result = grammar.elem:parse(buf:pos('begin'))

	assertEquals(result.foo:asstring(), "foo")
	assertEquals(result.bar:asstring(), "bar")
end

function TestGrammarBytes:test_byte_until_streamed()
	-- Given
	local grammar = haka.grammar.new("test", function ()
		elem = record{
			field("foo", bytes():untiltoken("bar")),
			field("bar", bytes()),
		}
		export(elem)
	end)

	-- When
	local result
	self:gen_stream({ "foob", "ar" }, function (iter)
		result = grammar.elem:parse(iter)
	end)


	assertEquals(result.foo:asstring(), "foo")
	assertEquals(result.bar:asstring(), "bar")
end

function TestGrammarBytes:test_byte_until_chunked()
	-- Given
	local chunks = {}
	local grammar = haka.grammar.new("test", function ()
		elem = record{
			bytes():untiltoken("bar")
				:chunked(function (result, sub, islast, context)
					if sub then
						table.insert(chunks, sub:asstring())
					end
				end),
			field("bar", bytes()),
		}
		export(elem)
	end)

	-- When
	local result
	self:gen_stream({ "av", "ery", "longf", "oob", "ar" }, function (iter)
		result = grammar.elem:parse(iter)
	end)

	assertEquals(result.bar:asstring(), "bar")
	assertEquals(table.concat(chunks), "averylongfoo")
end

function TestGrammarBytes:test_byte_until_partial_but_no()
	-- Given
	local chunks = {}
	local grammar = haka.grammar.new("test", function ()
		elem = record{
			bytes():untiltoken("bar")
				:chunked(function (result, sub, islast, context)
					if sub then
						table.insert(chunks, sub:asstring())
					end
				end),
			field("bar", bytes()),
		}
		export(elem)
	end)

	-- When
	local result
	self:gen_stream({ "is it a full ba", "? no just a bar" }, function (iter)
		result = grammar.elem:parse(iter)
	end)

	assertEquals(result.bar:asstring(), "bar")
	assertEquals(table.concat(chunks), "is it a full ba? no just a ")
end

addTestSuite('TestGrammarBytes')

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

TestGrammarEmpty = {}

function TestGrammarEmpty:test_empty_branch()
	-- Given
	local grammar = haka.grammar.new("human", function ()
		test = branch(
			{
				head = token('head'),
				body = empty()
			},
			function (self, ctx) return 'body' end
		)

		export(test)
	end)

	local buf = haka.vbuffer_from("head")

	-- When
	local res, err = grammar.test:parse(buf:pos('begin'))

	-- Then
	assertIsNil(err)
end

addTestSuite('TestGrammarEmpty')

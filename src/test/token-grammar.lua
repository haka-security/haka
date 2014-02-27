-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('protocol/ipv4')
require('protocol/tcp-connection')

local grammar = haka.grammar.record{
	haka.grammar.field("word1", haka.grammar.token("[^ \r\n]+")),
	haka.grammar.token(" "),
	haka.grammar.field("word2", haka.grammar.token("[^\r\n]+")),
	haka.grammar.token("[\r\n]*"),
}:compile()

haka.rule{
	-- Intercept tcp packets
	hook = haka.event('tcp-connection', 'receive_data'),
	eval = function (flow, input, direction)
		if direction == 'up' then
			local ctx = class('ctx'):new()
			local iter = input:pos("begin")
			if iter:available() > 0 then
				grammar:parseall(iter, ctx, nil)
				if ctx.word1 then
					print("word1= "..ctx.word1)
				end
				if ctx.word2 then
					print("word2= "..ctx.word2)
				end
			end
		end
	end
}

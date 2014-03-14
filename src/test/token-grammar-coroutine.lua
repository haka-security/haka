-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('protocol/ipv4')
require('protocol/tcp-connection')

local header = haka.grammar.record{
	haka.grammar.field("name", haka.grammar.token("[^:\r\n]+")),
	haka.grammar.token(": "),
	haka.grammar.field("value", haka.grammar.token("[^\r\n]+")),
	haka.grammar.token("[\r\n]+"),
}

local grammar = haka.grammar.record{
	haka.grammar.field("hello_world", haka.grammar.token("hello world")),
	haka.grammar.token("\r\n"),
	haka.grammar.field('headers', haka.grammar.array(header)
		:options{
			count = 3
		}),
}:compile()

haka.rule{
	-- Intercept tcp packets
	hook = haka.event('tcp-connection', 'receive_data'),
	streamed = true,
	eval = function (flow, iter, direction)
		if direction == 'up' then
			local ctx = class('ctx'):new()
			local mark = iter:copy()
			mark:mark()
			grammar:parse(iter, ctx)
			print("hello_world= "..ctx.hello_world)
			for _,header in ipairs(ctx.headers) do
				print(header.name..": "..header.value)
			end
			mark:unmark()
		end
	end
}

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
require('protocol/ipv4')
local tcp_connection = require('protocol/tcp_connection')

local grammar = haka.grammar.new("test", function ()
	grammar = record{
		field("word1", token("[^ \r\n]+")),
		token(" "),
		field("word2", token("[^\r\n]+")),
		token("[\r\n]*"),
	}

	export(grammar)
end)

haka.rule{
	-- Intercept tcp packets
	hook = tcp_connection.events.receive_data,
	eval = function (flow, input, direction)
		if direction == 'up' then
			local ctx = class.class('ctx'):new()
			local iter = input:pos("begin")
			if iter:available() > 0 then
				local ctx, err = grammar.grammar:parse(iter, ctx)
				if err then
					print("Parsing error : "..err)
					return
				end
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

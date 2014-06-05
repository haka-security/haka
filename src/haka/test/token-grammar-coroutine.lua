-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
require('protocol/ipv4')
local tcp_connection = require('protocol/tcp_connection')

local grammar = haka.grammar.new("test", function ()
	header = record{
		field("name", token("[^:\r\n]+")),
		token(": "),
		field("value", token("[^\r\n]+")),
		token("[\r\n]+"),
	}

	grammar = record{
		field("hello_world", token("hello world")),
		token("\r\n"),
		field('headers', array(header)
			:count(3)
		),
	}

	export(grammar)
end)

haka.rule{
	-- Intercept tcp packets
	hook = tcp_connection.events.receive_data,
	options = {
		streamed = true,
	},
	eval = function (flow, iter, direction)
		if direction == 'up' then
			local ctx = class.class('ctx'):new()
			local mark = iter:copy()
			mark:mark()
			grammar.grammar:parse(iter, ctx)
			print("hello_world= "..ctx.hello_world)
			for _,header in ipairs(ctx.headers) do
				print(header.name..": "..header.value)
			end
			mark:unmark()
		end
	end
}

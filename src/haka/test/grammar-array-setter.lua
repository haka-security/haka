-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

require('protocol/ipv4')
local tcp_connection = require('protocol/tcp_connection')

local grammar = haka.grammar.new("test", function ()
	elem = record{
		field("flags", number(3, 'big')),
		field("value", number(13, 'big'))
	}

	grammar = record{
		field("length", number(8, 'big')),
		field("data", array(elem)
			:untilcond(function (elem, ctx)
				return ctx.iter.meter-1 >= ctx:result(1).length
			end)
			:creation(function (entity, init)
				local vbuf = haka.vbuffer_allocate(2)
				return vbuf, entity:create(vbuf:pos('begin'), init)
			end)
		)
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
			if iter:available() > 0 then
				local ctx, err = grammar.grammar:parse(iter, ctx)
				if err then
					print("Parsing error : "..err)
					return
				end
				debug.pprint(ctx)
				ctx.data:remove(ctx.data[2])
				ctx.data:append({flags = 4, value = 5})
				debug.pprint(ctx)
			end
		end
	end
}

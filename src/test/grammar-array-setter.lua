-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('protocol/ipv4')
require('protocol/tcp-connection')

local elem = haka.grammar.record{
	haka.grammar.field("flags", haka.grammar.number(3, 'big')),
	haka.grammar.field("value", haka.grammar.number(13, 'big'))
}

local grammar = haka.grammar.record{
	haka.grammar.field("length", haka.grammar.number(8, 'big')),
	haka.grammar.field("data", haka.grammar.array(elem)
		:options{
			untilcond = function (elem, ctx)
				return ctx.iter.meter-1 >= ctx.top.length
			end,
			create = function (ctx, entity, init)
				local vbuf = haka.vbuffer(2)
				entity:create(vbuf:pos('begin'), ctx, init)
				return vbuf
			end
		})
}:compile()

haka.rule{
	-- Intercept tcp packets
	hook = haka.event('tcp-connection', 'receive_data'),
	streamed = true,
	eval = function (flow, iter, direction)
		if direction == 'up' then
			local ctx = class('ctx'):new()
			if iter:available() > 0 then
				local ctx, err = grammar:parse(iter, ctx)
				if err then
					print("Parsing error : "..err)
					return
				end
				haka.debug.pprint(ctx)
				ctx.data:remove(ctx.data[2])
				ctx.data:append({flags = 4, value = 5})
				haka.debug.pprint(ctx)
			end
		end
	end
}

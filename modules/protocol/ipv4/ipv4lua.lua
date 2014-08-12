-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

--local ipv4 = require('protocol/ipv4')

local ipv4_dissector = haka.dissector.new{
	type = haka.helper.EncapsulatedPacketDissector,
	name = 'ipv4'
}

local ipv4_addr_convert = {
	get = function (x) return ipv4.addr(x) end,
	set = function (x) return x.packed end
}

ipv4_dissector.grammar = haka.grammar.new("ipv4", function ()
	local option_header  = record{
		field('copy',        number(1)),
		field('class',       number(2)),
		field('number',      number(5)),
	}
	
	local option_data = record{
		field('len',         number(8))
			:validate(function (self) self.len = #self.data+2 end),
		field('data',        bytes()
			:count(function (self) return self.len-2 end)
		)
	}
	
	local option = record{
		union{
			option_header,
			field('type',    number(8))
		},
		optional(option_data,
			function (self) return self.type ~= 0 and self.type ~= 1 end
		)
	}
	
	local header = record{
		field('version',     number(4))
			:validate(function (self) self.version = 4 end),
		field('hdr_len',     number(4))
			:convert(converter.mult(4))
			:validate(function (self) self.hdr_len = self:_compute_hdr_len(self) end),
		field('tos',         number(8)),
		field('len',         number(16))
			:validate(function (self) self.len = self.hdr_len + #self.payload end),
		field('id',          number(16)),
		field('flags',       record{
			field('rb',      flag),
			field('df',      flag),
			field('mf',      flag),
		}),
		field('frag_offset', number(13))
			:convert(converter.mult(8)),
		field('ttl',         number(8)),
		field('proto',       number(8)),
		field('checksum',    number(16))
			:validate(function (self)
				self.checksum = 0
				self.checksum = ipv4.inet_checksum_compute(self._payload:sub(0, self.hdr_len))
			end),
		field('src',         number(32))
			:convert(ipv4_addr_convert, true),
		field('dst',         number(32))
			:convert(ipv4_addr_convert, true),
		field('opt',         array(option))
			:untilcond(function (elem, ctx)
				return ctx.iter.meter >= ctx:result(1).hdr_len or
					(elem and elem.type == 0)
			end),
		align(32),
		verify(function (self, ctx)
			if ctx.iter.meter ~= self.hdr_len then
				error(string.format("invalid ipv4 header size, expected %d bytes, got %d bytes", self.hdr_len, ctx.iter.meter))
			end
		end),
		field('payload',     bytes())
	}

	export(header)
end)

function ipv4_dissector.method:parse_payload(pkt, payload)
	self.raw = pkt
	ipv4_dissector.grammar:parse(payload:pos("begin"), self)
end

function ipv4_dissector.method:create_payload(pkt, payload, init)
	self.raw = pkt
	ipv4_dissector.grammar:create(payload:pos("begin"), self, init)
end

function ipv4_dissector.method:verify_checksum()
	return ipv4.inet_checksum_compute(self._payload:sub(0, self.hdr_len)) == 0
end

function ipv4_dissector.method:next_dissector()
	return ipv4.ipv4_protocol_dissectors[self.proto]
end

function ipv4_dissector._compute_hdr_len(pkt)
	local len = 20
	if pkt.opt then
		for _, opt in ipairs(pkt.opt) do
			len = len + (opt.len or 1)
		end
	end
	return len
end

function ipv4_dissector.method:forge_payload(pkt, payload)
	if payload.modified then
		self.len = nil
		self.checksum = nil
	end

	self:validate()
end

function ipv4_dissector:create(pkt, init)
	if not init then init = {} end
	if not init.hdr_len then init.hdr_len = ipv4_dissector._compute_hdr_len(init) end
	pkt.payload:append(haka.vbuffer_allocate(init.hdr_len))

	local ip = ipv4_dissector:new(pkt)
	ip:create(init, pkt)

	return ip
end

return ipv4

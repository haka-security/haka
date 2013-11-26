
--local ipv4 = require('protocol/ipv4')

local ipv4_dissector = haka.dissector.new{
	type = haka.dissector.EncapsulatedPacketDissector,
	name = 'ipv4'
}

local ipv4_addr_convert = {
	get = function (x) return ipv4.addr(x) end,
	set = function (x) return x.packed end
}

local option_header  = haka.grammar.record{
	haka.grammar.field('copy',        haka.grammar.number(1)),
	haka.grammar.field('class',       haka.grammar.number(2)),
	haka.grammar.field('number',      haka.grammar.number(5)),
}:extra{
	iseol = function (self)
		return self.copied == 0 and
			self.class == 0 and
			self.number == 0
	end
}

local option_data = haka.grammar.record{
	haka.grammar.field('len',         haka.grammar.number(8)),
	haka.grammar.field('data',        haka.grammar.bytes()
		:options{count = function (self) return self.len-2 end})
}

local option = haka.grammar.record{
	option_header,
	haka.grammar.optional(option_data,
		function (self) return not self.iseof end
	)
}

local header = haka.grammar.record{
	haka.grammar.field('version',     haka.grammar.number(4)),
	haka.grammar.field('hdr_len',     haka.grammar.number(4)
		:convert(haka.grammar.converter.mult(4))),
	haka.grammar.field('tos',         haka.grammar.number(8)),
	haka.grammar.field('len',         haka.grammar.number(16)),
	haka.grammar.field('id',          haka.grammar.number(16)),
	haka.grammar.field('flags',       haka.grammar.record{
		haka.grammar.field('rb', haka.grammar.flag),
		haka.grammar.field('df', haka.grammar.flag),
		haka.grammar.field('mf', haka.grammar.flag),
	}),
	haka.grammar.field('frag_offset', haka.grammar.number(13)
		:convert(haka.grammar.converter.mult(8))),
	haka.grammar.field('ttl',         haka.grammar.number(8)),
	haka.grammar.field('proto',       haka.grammar.number(8)),
	haka.grammar.field('checksum',    haka.grammar.number(16)),
	haka.grammar.field('src',         haka.grammar.number(32)
		:convert(ipv4_addr_convert, true)),
	haka.grammar.field('dst',         haka.grammar.number(32)
		:convert(ipv4_addr_convert, true)),
	haka.grammar.field('opt',         haka.grammar.array(option)
		:options{
			untilcond = function (elem, ctx)
				return ctx.offset >= ctx.top.hdr_len or (elem and elem.iseol)
			end
		}),
	haka.grammar.padding{align = 32},
	--haka.grammar.assert(function (self, ctx) return ctx.offset == self.hdr_len end, "invalid ipv4 header")
}

ipv4_dissector.grammar = header:compile()

function ipv4_dissector.method:parse_payload(pkt, payload, header_only)
	self.raw = pkt
	ipv4_dissector.grammar:parseall(payload, self)
	if not self.payload then
		self.payload = payload:right(self.hdr_len):extract(false)
	end
end

function ipv4_dissector.method:verify_checksum()
	return ipv4.inet_checksum(self._payload) == 0
end

function ipv4_dissector.method:compute_checksum()
	self.checksum = 0
	self.checksum = ipv4.inet_checksum(self._payload)
end

function ipv4_dissector.method:forge_payload(pkt, payload)
	local len = #payload + #self.payload
	if self.len ~= len then
		self.len = len
	end

	if payload.modified then
		self:compute_checksum()
	end

	payload:append(self.payload, false)
	self.payload = nil
end

function ipv4_dissector:create(pkt)
	pkt.payload:append(haka.vbuffer(20))

	local ip = ipv4_dissector:new(pkt)
	ip.payload = haka.vbuffer(0)
	ip:parse(pkt)

	ip.version = 4
	ip.checksum = 0
	ip.len = 0
	ip.hdr_len = 20
	return ip
end

return ipv4

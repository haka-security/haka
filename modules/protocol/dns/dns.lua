-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/udp-connection")
local ipv4 = require('protocol/ipv4')

local module = {}

local NO_COMPRESSION      = 0
local POINTER_COMPRESSION = 3

local TYPE = {
	'A',      -- a host address
	'NS',     -- an authoritative name server
	'MD',     -- a mail destination (Obsolete - use MX)
	'MF',     -- a mail forwarder (Obsolete - use MX)
	'CNAME',  -- the canonical name for an alias
	'SOA',    -- marks the start of a zone of authority
	'MB',     -- a mailbox domain name (EXPERIMENTAL)
	'MG',     -- a mail group member (EXPERIMENTAL)
	'MR',     -- a mail rename domain name (EXPERIMENTAL)
	'NULL',   -- a null RR (EXPERIMENTAL)
	'WKS',    -- a well known service description
	'PTR',    -- a domain name pointer
	'HINFO',  -- host information
	'MINFO',  -- mailbox or mail list information
	'MX',     -- mail exchange
	'TXT',    -- text strings
}

--
-- DNS Utils
--

local ipv4_addr_convert = {
	get = function (x) return ipv4.addr(x) end,
	set = function (x) return x.packed end
}

local function pointer_resolution(self, ctx)
	ctx = ctx.top
	for offset, label in sorted_pairs(ctx._labels) do
		if label.compression_scheme == POINTER_COMPRESSION then
			print(string.format("RESOLVE POINTER AT 0x%x (0x%x)", offset, offset+0x2A))
			if not ctx._labels[label.pointer] then
				error(string.format("Reference unknown domain name at offset: 0x%x (0x%x)", label.pointer, label.pointer+0x2A))
			end
			label.next = ctx._labels[label.pointer]
		end
	end
end

local dn_converter = {
	get = function (dn)
		local result = {}
		local label = dn[1]
		while label do
			-- We just skip pointer
			if label.name then
				table.insert(result, label.name:asstring())
			end
			label = label.next
		end
		return table.concat(result, ".")
	end,
	set = nil
}

--
-- DNS dissector
--

local dns_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'dns'
}

dns_dissector:register_event('request')
dns_dissector:register_event('response')

function dns_dissector.method:__init(flow)
	self.flow = flow
end

function dns_dissector.method:receive(payload, direction, pkt)
	local res = dns_dissector.grammar.message:parse(payload:pos("begin"))

	if direction == 'up' then
		self:trigger('request', res)
	else
		self:trigger('response', res)
	end

	pkt:send()
end

function dns_dissector.method:continue()
	self.flow:continue()
end

dns_dissector.grammar = haka.grammar:new("udp")

dns_dissector.grammar.label = haka.grammar.record{
	haka.grammar.execute(function (self, ctx)
		print(string.format("NEW LABEL AT 0x%x (0x%x)", ctx.iter.meter, ctx.iter.meter+0x2A))
		if #ctx.prev_result-1 > 0 then
			ctx.prev_result[#ctx.prev_result-1].next = self
		end
		ctx.top._labels[ctx.iter.meter] = self
	end),
	haka.grammar.field('compression_scheme', haka.grammar.number(2)),
	haka.grammar.field('length', haka.grammar.number(6)),
	haka.grammar.branch({
		name = haka.grammar.field('name', haka.grammar.bytes():options{
				count = function (self, ctx, el) return self.length end
			}),
		pointer = haka.grammar.field('_offset', haka.grammar.number(8)),
		default = haka.grammar.error("Unsupported compression scheme"),
		},
		function (self, ctx)
			if self.compression_scheme == POINTER_COMPRESSION then
				return 'pointer'
			elseif self.compression_scheme == NO_COMPRESSION then
				return 'name'
			end
		end
	)
}:extra{
	pointer = function (self)
		if self.compression_scheme ~= POINTER_COMPRESSION then
			return nil
		end

		return self.length * 256 + self._offset
	end
}

dns_dissector.grammar.dn = haka.grammar.array(dns_dissector.grammar.label):options{
		untilcond = function (label) return label and (label.length == 0 or label.pointer ~= nil) end,
}:convert(dn_converter, true)

dns_dissector.grammar.header = haka.grammar.record{
	haka.grammar.field('id',      haka.grammar.number(16)),
	haka.grammar.field('qr',      haka.grammar.flag),
	haka.grammar.field('opcode',  haka.grammar.number(4)),
	haka.grammar.field('aa',      haka.grammar.flag),
	haka.grammar.field('tc',      haka.grammar.flag),
	haka.grammar.field("rd",      haka.grammar.flag),
	haka.grammar.field("ra",      haka.grammar.flag),
	haka.grammar.field("z",       haka.grammar.number(3)),
	haka.grammar.field("rcode",   haka.grammar.number(4)),
	haka.grammar.field("qdcount", haka.grammar.number(16)),
	haka.grammar.field("ancount", haka.grammar.number(16)),
	haka.grammar.field("nscount", haka.grammar.number(16)),
	haka.grammar.field("arcount", haka.grammar.number(16)),
}

dns_dissector.grammar.question = haka.grammar.record{
	haka.grammar.field('name',    dns_dissector.grammar.dn),
	haka.grammar.field('type',    haka.grammar.number(16)),
	haka.grammar.field('class',   haka.grammar.number(16)),
}

dns_dissector.grammar.resourcerecord = haka.grammar.record{
	haka.grammar.field('name',    dns_dissector.grammar.dn),
	haka.grammar.field('type',    haka.grammar.number(16)),
	haka.grammar.field('class',   haka.grammar.number(16)),
	haka.grammar.field('ttl',     haka.grammar.number(32)),
	haka.grammar.field('length',  haka.grammar.number(16)),
	haka.grammar.branch({
		A =       haka.grammar.field('ip', haka.grammar.number(32)
			:convert(ipv4_addr_convert, true)),
		NS =      haka.grammar.field('name', dns_dissector.grammar.dn),
		MD =      haka.grammar.error("Unsupported type. Will come soon !"),
		MF =      haka.grammar.error("Unsupported type. Will come soon !"),
		CNAME =   haka.grammar.field('name', dns_dissector.grammar.dn),
		SOA =     haka.grammar.error("Unsupported type. Will come soon !"),
		MB =      haka.grammar.error("Unsupported type. Will come soon !"),
		MG =      haka.grammar.error("Unsupported type. Will come soon !"),
		MR =      haka.grammar.error("Unsupported type. Will come soon !"),
		NULL =    haka.grammar.field('data', haka.grammar.bytes():options{
			count = function (self, ctx) return self.length end
		}),
		WKS =     haka.grammar.error("Unsupported type. Will come soon !"),
		PTR =     haka.grammar.error("Unsupported type. Will come soon !"),
		HINFO =   haka.grammar.error("Unsupported type. Will come soon !"),
		MINFO =   haka.grammar.error("Unsupported type. Will come soon !"),
		MX =      haka.grammar.error("Unsupported type. Will come soon !"),
		TXT =     haka.grammar.field('data', haka.grammar.bytes():options{
			count = function (self, ctx) return self.length end
		}),
		default = haka.grammar.error("Unsupported type."),
	},
	function (self, ctx)
		return TYPE[self.type]
	end
	),
}

dns_dissector.grammar.answer = haka.grammar.array(dns_dissector.grammar.resourcerecord):options{
	count = function (self, ctx) return ctx.top.header.ancount end
}

dns_dissector.grammar.authority = haka.grammar.array(dns_dissector.grammar.resourcerecord):options{
	count = function (self, ctx) return ctx.top.header.nscount end
}

dns_dissector.grammar.additional = haka.grammar.array(dns_dissector.grammar.resourcerecord):options{
	count = function (self, ctx) return ctx.top.header.arcount end
}

dns_dissector.grammar.message = haka.grammar.record{
	haka.grammar.execute(function (self, ctx) ctx.top._labels = {} end),
	haka.grammar.field('header',   dns_dissector.grammar.header),
	haka.grammar.field('question',   dns_dissector.grammar.question),
	haka.grammar.field('answer',    dns_dissector.grammar.answer),
	haka.grammar.field('authority',    dns_dissector.grammar.authority),
	haka.grammar.field('additional',    dns_dissector.grammar.additional),
	haka.grammar.execute(pointer_resolution),
}:compile()

function module.install_udp_rule(port)
	haka.rule{
		hook = haka.event('udp-connection', 'new_connection'),
		eval = function (flow, pkt)
			if pkt.dstport == port then
				haka.log.debug('dns', "selecting dns dissector on flow")
				flow:select_next_dissector(dns_dissector:new(flow))
			end
		end
	}
end

return module

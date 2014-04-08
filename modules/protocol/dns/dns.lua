-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/udp-connection")

local module = {}

local NO_COMPRESSION      = 0
local POINTER_COMPRESSION = 3

local TYPE_A     =  1 -- a host address
local TYPE_NS    =  2 -- an authoritative name server
local TYPE_MD    =  3 -- a mail destination (Obsolete - use MX)
local TYPE_MF    =  4 -- a mail forwarder (Obsolete - use MX)
local TYPE_CNAME =  5 -- the canonical name for an alias
local TYPE_SOA   =  6 -- marks the start of a zone of authority
local TYPE_MB    =  7 -- a mailbox domain name (EXPERIMENTAL)
local TYPE_MG    =  8 -- a mail group member (EXPERIMENTAL)
local TYPE_MR    =  9 -- a mail rename domain name (EXPERIMENTAL)
local TYPE_NULL  = 10 -- a null RR (EXPERIMENTAL)
local TYPE_WKS   = 11 -- a well known service description
local TYPE_PTR   = 12 -- a domain name pointer
local TYPE_HINFO = 13 -- host information
local TYPE_MINFO = 14 -- mailbox or mail list information
local TYPE_MX    = 15 -- mail exchange
local TYPE_TXT   = 16 -- text strings

--
-- DNS Utils
--

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
	haka.grammar.field('data',    haka.grammar.branch({
		A =       haka.grammar.number(32)
			:convert(ipv4_addr_convert, true),
		NS =      dns_dissector.grammar.dn,
		MD =      haka.grammar.error("Unsupported type. Will come soon !"),
		MF =      haka.grammar.error("Unsupported type. Will come soon !"),
		CNAME =   dns_dissector.grammar.dn,
		SOA =     haka.grammar.error("Unsupported type. Will come soon !"),
		MB =      haka.grammar.error("Unsupported type. Will come soon !"),
		MG =      haka.grammar.error("Unsupported type. Will come soon !"),
		MR =      haka.grammar.error("Unsupported type. Will come soon !"),
		NULL =    haka.grammar.bytes():options{
			count = function (self, ctx) return self.length end
		},
		WKS =     haka.grammar.error("Unsupported type. Will come soon !"),
		PTR =     haka.grammar.error("Unsupported type. Will come soon !"),
		HINFO =   haka.grammar.error("Unsupported type. Will come soon !"),
		MINFO =   haka.grammar.error("Unsupported type. Will come soon !"),
		MX =      haka.grammar.error("Unsupported type. Will come soon !"),
		TXT =     haka.grammar.bytes():options{
			count = function (self, ctx) return self.length end
		},
		default = haka.grammar.error("Unsupported type."),
	},
	function (self, ctx)
		if self.type == TYPE_A then
			return 'A'
		elseif self.type == TYPE_NS then
			return 'NS'
		elseif self.type == TYPE_MD then
			return 'MD'
		elseif self.type == TYPE_MF then
			return 'MF'
		elseif self.type == TYPE_CNAME then
			return 'CNAME'
		elseif self.type == TYPE_SOA then
			return 'SOA'
		elseif self.type == TYPE_MB then
			return 'MB'
		elseif self.type == TYPE_MG then
			return 'MG'
		elseif self.type == TYPE_MR then
			return 'MR'
		elseif self.type == TYPE_NULL then
			return 'NULL'
		elseif self.type == TYPE_WKS then
			return 'WKS'
		elseif self.type == TYPE_PTR then
			return 'PTR'
		elseif self.type == TYPE_HINFO then
			return 'HINFO'
		elseif self.type == TYPE_MINFO then
			return 'MINFO'
		elseif self.type == TYPE_MX then
			return 'MX'
		elseif self.type == TYPE_TXT then
			return 'TXT'
		end
	end
	)),
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

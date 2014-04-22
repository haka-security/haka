-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/udp-connection")
local ipv4 = require('protocol/ipv4')

local module = {}

local NO_COMPRESSION      = 0
local POINTER_COMPRESSION = 3

local TYPE = {
	 [1] = 'A',      -- a host address
	 [2] = 'NS',     -- an authoritative name server
	 [3] = 'MD',     -- a mail destination (Obsolete - use MX)
	 [4] = 'MF',     -- a mail forwarder (Obsolete - use MX)
	 [5] = 'CNAME',  -- the canonical name for an alias
	 [6] = 'SOA',    -- marks the start of a zone of authority
	 [7] = 'MB',     -- a mailbox domain name (EXPERIMENTAL)
	 [8] = 'MG',     -- a mail group member (EXPERIMENTAL)
	 [9] = 'MR',     -- a mail rename domain name (EXPERIMENTAL)
	[10] = 'NULL',   -- a null RR (EXPERIMENTAL)
	[11] = 'WKS',    -- a well known service description
	[12] = 'PTR',    -- a domain name pointer
	[13] = 'HINFO',  -- host information
	[14] = 'MINFO',  -- mailbox or mail list information
	[15] = 'MX',     -- mail exchange
	[16] = 'TXT',    -- text strings
	[28] = 'AAAA',   -- a host ipv6 address
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
			if not ctx._labels[label.pointer] then
				error(string.format("reference unknown domain name at offset: 0x%x", label.pointer))
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

dns_dissector:register_event('query')
dns_dissector:register_event('response')

function dns_dissector.method:__init(flow)
	self.flow = flow
	self.states = dns_dissector.states:instanciate()
	self.states.dns = self
end

function dns_dissector.method:receive(pkt, payload, direction)
	self.states:update(payload, direction, pkt)
end

function dns_dissector.method:continue()
	self.flow:continue()
end

dns_dissector.grammar = haka.grammar:new("udp")

dns_dissector.grammar.label = haka.grammar.record{
	haka.grammar.execute(function (self, ctx)
		if #ctx.prev_result-1 > 0 then
			ctx.prev_result[#ctx.prev_result-1].next = self
		end
		ctx.top._labels[ctx.iter.meter] = self
	end),
	haka.grammar.field('compression_scheme', haka.grammar.number(2)),
	haka.grammar.branch({
		name = haka.grammar.record{
				haka.grammar.field('length', haka.grammar.number(6)),
				haka.grammar.field('name', haka.grammar.bytes():options{
					count = function (self, ctx, el) return self.length end
				}),
			},
		pointer = haka.grammar.field('pointer', haka.grammar.number(14)),
		default = haka.grammar.error("unsupported compression scheme"),
		},
		function (self, ctx)
			if self.compression_scheme == POINTER_COMPRESSION then
				return 'pointer'
			elseif self.compression_scheme == NO_COMPRESSION then
				return 'name'
			end
		end
	)
}

dns_dissector.grammar.dn = haka.grammar.array(dns_dissector.grammar.label):options{
		untilcond = function (label)
			return label and (label.length == 0 or label.pointer ~= nil)
		end,
}:convert(dn_converter, true)

dns_dissector.type = haka.grammar.number(16):convert({
	get = function (type) return TYPE[type] or type end,
	set = function (type)
		for i, type in TYPE do
			if type == type then
				return i
			end
		end
		error("unknown type: "..type)
	end
})

dns_dissector.grammar.header = haka.grammar.record{
	haka.grammar.field('id',      haka.grammar.number(16)),
	haka.grammar.field('qr',      haka.grammar.flag),
	haka.grammar.field('opcode',  haka.grammar.number(4)),
	haka.grammar.field('aa',      haka.grammar.flag),
	haka.grammar.field('tc',      haka.grammar.flag),
	haka.grammar.field("rd",      haka.grammar.flag),
	haka.grammar.field("ra",      haka.grammar.flag),
	haka.grammar.number(3),
	haka.grammar.field("rcode",   haka.grammar.number(4)),
	haka.grammar.field("qdcount", haka.grammar.number(16)),
	haka.grammar.field("ancount", haka.grammar.number(16)),
	haka.grammar.field("nscount", haka.grammar.number(16)),
	haka.grammar.field("arcount", haka.grammar.number(16)),
}

dns_dissector.grammar.question = haka.grammar.record{
	haka.grammar.field('name',    dns_dissector.grammar.dn),
	haka.grammar.field('type',    dns_dissector.type),
	haka.grammar.field('class',   haka.grammar.number(16)),
}

dns_dissector.grammar.soa = haka.grammar.record{
	haka.grammar.field('mname', dns_dissector.grammar.dn),
	haka.grammar.field('rname', dns_dissector.grammar.dn),
	haka.grammar.field('serial', haka.grammar.number(32)),
	haka.grammar.field('refresh', haka.grammar.number(32)),
	haka.grammar.field('retry', haka.grammar.number(32)),
	haka.grammar.field('expire', haka.grammar.number(32)),
	haka.grammar.field('minimum', haka.grammar.number(32)),
}

dns_dissector.grammar.wks = haka.grammar.record{
	haka.grammar.field('ip', haka.grammar.number(32)
		:convert(ipv4_addr_convert, true)),
	haka.grammar.field('proto', haka.grammar.number(8)),
	haka.grammar.field('ports', haka.grammar.bytes():options{
		count = function (self, ctx) return self.length - 40 end -- remove wks headers
	}),
}

dns_dissector.grammar.mx = haka.grammar.record{
	haka.grammar.field('pref', haka.grammar.number(16)),
	haka.grammar.field('name', dns_dissector.grammar.dn),
}

dns_dissector.grammar.minfo = haka.grammar.record{
	haka.grammar.field('rname', dns_dissector.grammar.dn),
	haka.grammar.field('ename', dns_dissector.grammar.dn),
}

dns_dissector.grammar.resourcerecord = haka.grammar.record{
	haka.grammar.field('name',    dns_dissector.grammar.dn),
	haka.grammar.field('type',    dns_dissector.type),
	haka.grammar.field('class',   haka.grammar.number(16)),
	haka.grammar.field('ttl',     haka.grammar.number(32)),
	haka.grammar.field('length',  haka.grammar.number(16)),
	haka.grammar.branch({
			A =       haka.grammar.field('ip', haka.grammar.number(32)
				:convert(ipv4_addr_convert, true)),
			NS =      haka.grammar.field('name', dns_dissector.grammar.dn),
			MD =      haka.grammar.field('name', dns_dissector.grammar.dn),
			MF =      haka.grammar.field('name', dns_dissector.grammar.dn),
			CNAME =   haka.grammar.field('name', dns_dissector.grammar.dn),
			SOA =     dns_dissector.grammar.soa,
			MB =      haka.grammar.field('name', dns_dissector.grammar.dn),
			MG =      haka.grammar.field('name', dns_dissector.grammar.dn),
			MR =      haka.grammar.field('name', dns_dissector.grammar.dn),
			NULL =    haka.grammar.field('data', haka.grammar.bytes():options{
				count = function (self, ctx) return self.length end
			}),
			WKS =     dns_dissector.grammar.wks,
			PTR =     haka.grammar.field('name', dns_dissector.grammar.dn),
			MINFO =   dns_dissector.grammar.minfo,
			MX =      dns_dissector.grammar.mx,
			TXT =     haka.grammar.field('data', haka.grammar.text:options{
				count = function (self, ctx) return self.length end
			}),
			default = haka.grammar.field('unknown', haka.grammar.bytes():options{
				count = function (self, ctx) return self.length end
			}),
		},
		function (self, ctx)
			return self.type
		end
	),
}

dns_dissector.grammar.answer = haka.grammar.array(dns_dissector.grammar.resourcerecord):options{
	count = function (self, ctx) return ctx.top.ancount end
}

dns_dissector.grammar.authority = haka.grammar.array(dns_dissector.grammar.resourcerecord):options{
	count = function (self, ctx) return ctx.top.nscount end
}

dns_dissector.grammar.additional = haka.grammar.array(dns_dissector.grammar.resourcerecord):options{
	count = function (self, ctx) return ctx.top.arcount end
}

dns_dissector.grammar.message = haka.grammar.record{
	haka.grammar.execute(function (self, ctx) ctx.top._labels = {} end),
	dns_dissector.grammar.header,
	haka.grammar.field('question',   dns_dissector.grammar.question),
	haka.grammar.field('answer',    dns_dissector.grammar.answer),
	haka.grammar.field('authority',    dns_dissector.grammar.authority),
	haka.grammar.field('additional',    dns_dissector.grammar.additional),
	haka.grammar.execute(pointer_resolution),
}

local dns_message = dns_dissector.grammar.message:compile()

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

--
-- DNS States
--

dns_dissector.states = haka.state_machine("dns")

local dns_pending_queries = {}

dns_dissector.states:default{
	error = function (context)
		local self = context.dns
		self.flow:drop()
	end,
	update = function (context, payload, direction, pkt)
		local res, err = dns_message:parse(payload:pos('begin'))
		if err then
			haka.alert{
				description = string.format("invalid dns %s", err.rule),
				severity = 'low'
			}
			context.dns.flow:drop(pkt)
		else
			return context[direction](context, res, payload, pkt)
		end
	end
}

dns_dissector.states.message = dns_dissector.states:state{
	up = function (context, res, payload, pkt)
		local self = context.dns
		self:trigger("query", res)
		dns_pending_queries[res.id] = res
		res._data = self.flow:send(pkt, payload, true)
	end,
	down = function (context, res, payload, pkt)
		local self = context.dns
		local id = res.id
		local query = dns_pending_queries[id]
		if not query then
			haka.alert{
				description = "dns: mismatching response",
				severity = 'low'
			}
			self.flow:drop(pkt)
		else
			self:trigger("response", res, query)
			dns_pending_queries[id] = nil
			res._data = self.flow:send(pkt, payload, true)
		end
	end,
}

dns_dissector.states.initial = dns_dissector.states.message

return module

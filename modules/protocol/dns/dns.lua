-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local udp_connection = require("protocol/udp_connection")
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
	ctx = ctx:result(1)
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

local DnsResult = class.class('DnsResult')

function DnsResult.method:drop()
	self._dissector.flow:drop(self._pkt)
end

--
-- DNS dissector
--

local dns_dissector = haka.dissector.new{
	type = haka.dissector.FlowDissector,
	name = 'dns'
}

local function continue(self, res)
	return self:continue() or res._pkt:continue()
end

dns_dissector:register_event('query', continue)
dns_dissector:register_event('response', continue)

function dns_dissector.method:__init(flow)
	self.flow = flow
	self.state_machine = dns_dissector.state_machine:instanciate(self)
	self.dns_pending_queries = {}
end

function dns_dissector.method:receive(pkt, payload, direction)
	self.state_machine:update(payload:pos('begin'), direction, pkt, payload)
end

function dns_dissector.method:continue()
	self.flow:continue()
end

dns_dissector.grammar = haka.grammar.new("dns", function ()
	label = record{
		execute(function (self, ctx)
			local prev_result = ctx:result(-2)
			if #prev_result-1 > 0 then
				prev_result[#prev_result-1].next = self
			end
			ctx:result(1)._labels[ctx.iter.meter] = self
		end),
		field('compression_scheme', number(2)),
		branch({
			name = record{
					field('length', number(6)),
					field('name', bytes()
						:count(function (self, ctx, el) return self.length end)
					),
				},
			pointer = field('pointer', number(14)),
			default = fail("unsupported compression scheme"),
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

	dn = array(label)
		:untilcond(function (label)
			return label and (label.length == 0 or label.pointer ~= nil)
		end)
		:convert(dn_converter, true)

	dns_dissector.type = number(16)
		:convert({
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

	header = record{
		field('id',      number(16)),
		field('qr',      flag),
		field('opcode',  number(4)),
		field('aa',      flag),
		field('tc',      flag),
		field("rd",      flag),
		field("ra",      flag),
		number(3),
		field("rcode",   number(4)),
		field("qdcount", number(16)),
		field("ancount", number(16)),
		field("nscount", number(16)),
		field("arcount", number(16)),
	}

	question = record{
		field('name',    dn),
		field('type',    dns_dissector.type),
		field('class',   number(16)),
	}

	soa = record{
		field('mname',   dn),
		field('rname',   dn),
		field('serial',  number(32)),
		field('refresh', number(32)),
		field('retry',   number(32)),
		field('expire',  number(32)),
		field('minimum', number(32)),
	}

	wks = record{
		field('ip',    number(32)
			:convert(ipv4_addr_convert, true)),
		field('proto', number(8)),
		field('ports', bytes()
			:count(function (self, ctx) return self.length - 40 end) -- remove wks headers
		),
	}

	mx = record{
		field('pref', number(16)),
		field('name', dn),
	}

	minfo = record{
		field('rname', dn),
		field('ename', dn),
	}

	resourcerecord = record{
		field('name',    dn),
		field('type',    dns_dissector.type),
		field('class',   number(16)),
		field('ttl',     number(32)),
		field('length',  number(16)),
		branch({
				A =       field('ip', number(32)
					:convert(ipv4_addr_convert, true)),
				NS =      field('name', dn),
				MD =      field('name', dn),
				MF =      field('name', dn),
				CNAME =   field('name', dn),
				SOA =     soa,
				MB =      field('name', dn),
				MG =      field('name', dn),
				MR =      field('name', dn),
				NULL =    field('data', bytes()
					:count(function (self, ctx) return self.length end)
				),
				WKS =     wks,
				PTR =     field('name', dn),
				MINFO =   minfo,
				MX =      mx,
				TXT =     field('data', text
					:count(function (self, ctx) return self.length end)
				),
				default = field('unknown', bytes()
					:count(function (self, ctx) return self.length end)
				),
			},
			function (self, ctx)
				return self.type
			end
		),
	}

	answer = array(resourcerecord)
		:count(function (self, ctx) return ctx:result(1).ancount end)

	authority = array(resourcerecord)
		:count(function (self, ctx) return ctx:result(1).nscount end)

	additional = array(resourcerecord)
		:count(function (self, ctx) return ctx:result(1).arcount end)

	message = record{
		execute(function (self, ctx) ctx:result(1)._labels = {} end),
		header,
		field('question',   question),
		field('answer',     answer),
		field('authority',  authority),
		field('additional', additional),
		execute(pointer_resolution),
	}:result(DnsResult)

	export(message)
end)

function module.dissect(flow)
	flow:select_next_dissector(dns_dissector:new(flow))
end

function module.install_udp_rule(port)
	haka.rule{
		name = "install dns dissector",
		hook = udp_connection.events.new_connection,
		eval = function (flow, pkt)
			if pkt.dstport == port then
				haka.log.debug('dns', "selecting dns dissector on flow")
				module.dissect(flow)
			end
		end
	}
end

--
-- DNS States
--

dns_dissector.state_machine = haka.state_machine.new("dns", function ()
	state_type(BidirectionnalState)

	message = state(dns_dissector.grammar.message, dns_dissector.grammar.message)

	any:on{
		event = events.error,
		execute = function (self)
			self.flow:drop()
		end,
	}

	message:on{
		event = events.up,
		execute = function (self, res, pkt, payload)
			-- make pkt available to rules. Mainly for dropping
			res._pkt = pkt
			res._dissector = self
			self:trigger("query", res)
			self.dns_pending_queries[res.id] = res
			self.flow:send(pkt, payload, true)
			res._data = payload
		end,
	}

	message:on{
		event = events.down,
		execute = function (self, res, pkt, payload)
			-- make pkt available to rules. Mainly for dropping
			res._pkt = pkt
			res._dissector = self
			local id = res.id
			local query = self.dns_pending_queries[id]
			if not query then
				haka.alert{
					description = "dns: mismatching response",
					severity = 'low'
				}
				self.flow:drop(pkt)
			else
				self:trigger("response", res, query)
				self.dns_pending_queries[id] = nil
				self.flow:send(pkt, payload)
			end
		end,
	}

	initial(message)
end)

module.events = dns_dissector.events

return module

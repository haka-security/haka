-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/udp")

local module = {}

--
-- DNS dissector
--

local dns_dissector = haka.dissector.new{
	type = haka.dissector.EncapsulatedPacketDissector,
	name = 'dns'
}

dns_dissector:register_event('request')
dns_dissector:register_event('response')

function module.install_udp_rule(port)
	haka.rule{
		hook = haka.event('udp', 'receive_packet'),
		eval = function (pkt)
			if pkt.dstport == port or pkt.srcport == port then
				haka.log.debug('dns', "selecting dns dissector on packet")
				local pkt = dns_dissector.grammar.message:parse(pkt.payload:pos("begin"), self)
				debug.pprint(pkt, nil, nil, { debug.hide_underscore, debug.hide_function })
				print(pkt.question.name)
				if pkt.header.qr then
					print("Answer")
					for i, rr in ipairs(pkt.answer) do
						print(rr.name)
					end
					print("Authority")
					for i, rr in ipairs(pkt.authority) do
						print(rr.name)
					end
					print("Additional")
					for i, rr in ipairs(pkt.additional) do
						print(rr.name)
					end
				end
			end
		end
	}
end

function dns_dissector.method:parse_payload(pkt, payload)
	local pkt = dns_dissector.grammar.message:parse(payload:pos("begin"), self)
end

function dns_dissector.method:create_payload(pkt, payload, init)
	dns_dissector.grammar.packet:create(payload:pos("begin"), self, init)
end

function dns_dissector.method:forge_payload(pkt, payload)
	self:validate()
end

local DomainNameResult = class("DomainNameResult", haka.grammar.ArrayResult)

function DomainNameResult.method:__tostring()
	local result = {}
	for i, label in ipairs(self) do
		table.insert(result, label.name:asstring())
	end

	return table.concat(result, ".")
end

dns_dissector.grammar = haka.grammar:new("udp")

dns_dissector.grammar.label = haka.grammar.record{
	haka.grammar.union{
		haka.grammar.field('pointer', haka.grammar.record{
			haka.grammar.field('isa', haka.grammar.number(2)),
			haka.grammar.field('msb', haka.grammar.number(6)),
		}),
		haka.grammar.field('length', haka.grammar.number(8)),
	},
	haka.grammar.branch({
		name = haka.grammar.field('name', haka.grammar.bytes():options{
			count = function (self, ctx, el) return self.length end
		}),
		pointer = haka.grammar.field('offset', haka.grammar.number(8))
		:convert(dns_dissector.pointer_converter),
	},
	function (self, ctx)
		if self.pointer == 3 then
			return 'pointer'
		else
			return 'name'
		end
	end)
}

dns_dissector.grammar.domainname = haka.grammar.array(dns_dissector.grammar.label):options{
		untilcond = function (label) return label and (label.length == 0 or label.pointer == 3) end,
		result = DomainNameResult
	}

dns_dissector.grammar.header = haka.grammar.record{
	haka.grammar.field('id', haka.grammar.number(16)),
	haka.grammar.field('qr', haka.grammar.flag),
	haka.grammar.field('opcode', haka.grammar.number(4)),
	haka.grammar.field('aa', haka.grammar.flag),
	haka.grammar.field('tc', haka.grammar.flag),
	haka.grammar.field("rd", haka.grammar.flag),
	haka.grammar.field("ra", haka.grammar.flag),
	haka.grammar.field("z", haka.grammar.number(3)),
	haka.grammar.field("rcode", haka.grammar.number(4)),
	haka.grammar.field("qdcount", haka.grammar.number(16)),
	haka.grammar.field("ancount", haka.grammar.number(16)),
	haka.grammar.field("nscount", haka.grammar.number(16)),
	haka.grammar.field("arcount", haka.grammar.number(16)),
}

dns_dissector.grammar.question = haka.grammar.record{
	haka.grammar.field('name', dns_dissector.grammar.domainname),
	haka.grammar.field('type', haka.grammar.number(16)),
	haka.grammar.field('class', haka.grammar.number(16)),
}

dns_dissector.grammar.resourcerecord = haka.grammar.record{
	haka.grammar.field('name', dns_dissector.grammar.domainname),
	haka.grammar.field('type', haka.grammar.number(16)),
	haka.grammar.field('class', haka.grammar.number(16)),
	haka.grammar.field('ttl', haka.grammar.number(32)),
	haka.grammar.field('length', haka.grammar.number(16)),
	haka.grammar.field('data', haka.grammar.bytes():options{
		count = function (self, ctx) return self.length end
	})
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
	haka.grammar.field('header',   dns_dissector.grammar.header),
	haka.grammar.field('question',   dns_dissector.grammar.question),
	haka.grammar.field('answer',    dns_dissector.grammar.answer),
	haka.grammar.field('authority',    dns_dissector.grammar.authority),
	haka.grammar.field('additional',    dns_dissector.grammar.additional)
}:compile()

return module

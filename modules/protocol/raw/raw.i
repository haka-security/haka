/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

%module raw

%include "haka/lua/swig.si"
%include "haka/lua/packet.si"
%include "haka/lua/object.si"

%{
#include <haka/packet.h>
#include <haka/capture_module.h>
#include <haka/error.h>
%}

%nodefaultctor;
%nodefaultdtor;

const char *packet_dissector(struct packet *pkt);

%luacode{
	local this = unpack({...})

	local raw_dissector = haka.dissector.new{
		type = haka.helper.PacketDissector,
		name = 'raw'
	}

	haka.policy {
		on = raw_dissector.policies.next_dissector,
		name = "unknown dissector",
		action = function (policy, ctx, values, desc)
			haka.alert{
				description = string.format("dropping unknown dissector '%s'", values.proto)
			}
			return ctx:drop()
		end
	}

	haka.policy {
		on = raw_dissector.policies.next_dissector,
		name = "unknown protocol",
		proto = "unknown",
		action = haka.policy.accept
	}

	function raw_dissector.method:receive()
		haka.context:signal(self, raw_dissector.events.receive_packet)

		local protocol = this.packet_dissector(self)
		raw_dissector.policies.next_dissector:apply{
			values = {
				proto = protocol or 'unknown'
			},
			ctx = self,
		}
		local next_dissector = self:activate_next_dissector()
		if next_dissector then
			return next_dissector:preceive()
		else
			if self:can_continue() then
				return self:send()
			end
		end
	end

	function raw_dissector:new(pkt)
		return pkt
	end

	function raw_dissector:create(pkt, size)
		return haka.packet(size or 0)
	end

	local packet_send = swig.getclassmetatable('packet')['.fn'].send

	function raw_dissector.method:send()
		haka.context:signal(self, raw_dissector.events.send_packet)
		return packet_send(self)
	end

	function raw_dissector.method:can_continue()
		return self:issent()
	end

	swig.getclassmetatable('packet')['.fn'].send = raw_dissector.method.send
	swig.getclassmetatable('packet')['.fn'].receive = raw_dissector.method.receive
	swig.getclassmetatable('packet')['.fn'].continue = haka.helper.Dissector.method.continue
	swig.getclassmetatable('packet')['.fn'].can_continue = raw_dissector.method.can_continue
	swig.getclassmetatable('packet')['.fn'].error = swig.getclassmetatable('packet')['.fn'].drop
	swig.getclassmetatable('packet')['.fn'].select_next_dissector = raw_dissector.method.select_next_dissector
	swig.getclassmetatable('packet')['.fn'].activate_next_dissector = raw_dissector.method.activate_next_dissector

	function haka.filter(pkt)
		pkt:receive()
	end
}

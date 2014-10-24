-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with module
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <haka/config.h>

local module = {}

local raw_dissector = haka.dissector.new{
	type = haka.helper.PacketDissector,
	name = 'raw'
}

local dissectors = {}

#ifdef HAKA_FFI

local ffi = require('ffi')
ffi.cdef[[
	const char *packet_dissector(struct packet *pkt);

	struct time {
		int32_t      secs;   /**< Seconds */
		uint32_t     nsecs;  /**< Nano-seconds */
	};

	int packet_send(struct packet *pkt);
	void packet_inject(struct packet *pkt);
	void packet_drop(struct packet *pkt);
	const struct time *packet_timestamp(struct packet *pkt);
	struct vbuffer *packet_payload(struct packet *pkt);
	unsigned long long packet_id(struct packet *pkt);
	struct packet *packet_from_userdata(void *pkt);
	int32_t time_sec(const struct time *t);

	struct packet { };
]]

module.packet_dissector = function(self)
	local dissector = ffi.C.packet_dissector(self)
	if dissector == nil then
		return nil
	end
	return ffi.string(dissector)
end

local packet_send = ffi.C.packet_send

local method = {
	drop = function () return ffi.C.packet_drop end,
	inject = function () return ffi.C.packet_inject end,
	timestamp = ffi.C.packet_timestamp,
	payload = ffi.C.packet_payload,
	id = ffi.C.packet_id,
	send = function () return raw_dissector.method.send end,
	receive = function () return raw_dissector.method.receive end,
	continue = function () return haka.helper.Dissector.method.continue end,
	can_continue = function () return raw_dissector.method.can_continue end,
	error = function () return ffi.C.packet_drop end,
	name = function () return "raw" end,
}

local mt = {
	__index = function (table, key)
		local res = method[key]
		if res then
			return res(table)
		else
			return nil
		end
	end
}
ffi.metatype("struct packet", mt)

local method = {
	seconds = ffi.C.time_sec,
}

local mt = {
	__index = function (table, key)
		local res = method[key]
		if res then
			return res(table)
		else
			return nil
		end
	end
}
ffi.metatype("struct time", mt)

#else
#endif

function module.register(name, dissector)
	dissectors[name] = dissector
end

raw_dissector.options.drop_unknown_dissector = false

function raw_dissector.method:receive()
	haka.context:signal(self, raw_dissector.events.receive_packet)

	local dissector = module.packet_dissector(self)
	if dissector then
		local next_dissector = dissectors[dissector]
		if next_dissector then
			return next_dissector:receive(self)
		else
			if raw_dissector.options.drop_unknown_dissector then
				haka.log.error("dissector '%s' is unknown", dissector)
				return self:drop()
			else
				return self:send()
			end
		end
	else
		return self:send()
	end
end

function raw_dissector:new(pkt)
	return pkt
end

function raw_dissector:create(size)
	return haka.packet(size or 0)
end

function raw_dissector.method:send()
	haka.context:signal(self, raw_dissector.events.send_packet)
	return packet_send(self)
end

function raw_dissector.method:can_continue()
	return self:issent()
end

function haka.filter(pkt)
	raw_dissector:receive(pkt)
end

function module.create(size)
	return raw_dissector:create(size)
end

module.events = raw_dissector.events
module.options = raw_dissector.options

return module

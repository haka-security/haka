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
local packet_send

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
	return packet_new(size or 0)
end

function raw_dissector.method:send()
	haka.context:signal(self, raw_dissector.events.send_packet)
	return packet_send(self)
end

function raw_dissector.method:can_continue()
	return self:issent()
end

#ifdef HAKA_FFI

local ffibinding = require("ffibinding")
local lib = ffibinding.load()
local ffi = require('ffi')
ffi.cdef[[
	const char *packet_dissector(struct packet *pkt);

	struct time {
	};

	bool time_tostring(const struct time *t, char *buffer, size_t len);

	enum packet_status {
		normal, /**< Packet captured by Haka. */
		forged, /**< Packet forged. */
		sent,   /**< Packet already sent on the network. */
	};

	int packet_send(struct packet *pkt);
	void packet_inject(struct packet *pkt);
	void packet_drop(struct packet *pkt);
	const struct time *packet_timestamp(struct packet *pkt);
	struct vbuffer *packet_payload(struct packet *pkt);
	uint64_t packet_id(struct packet *pkt);
	struct packet *packet_from_userdata(void *pkt);
	double time_sec(const struct time *t);
	struct packet *packet_new(size_t size);
	enum packet_status packet_state(struct packet *pkt);

	struct packet { };
]]

module.packet_dissector = function(self)
	local dissector = ffi.C.packet_dissector(self)
	if dissector == nil then
		return nil
	end
	return ffi.string(dissector)
end

packet_send = ffi.C.packet_send

local prop = {
	timestamp = ffibinding.handle_error(ffi.C.packet_timestamp),
	payload = ffibinding.handle_error(ffi.C.packet_payload),
	id = function(self) return tonumber(ffibinding.handle_error(ffi.C.packet_id)(self)) end,
	state = ffibinding.handle_error(ffi.C.packet_state),
}

local meth = {
	drop = ffibinding.handle_error(ffi.C.packet_drop),
	inject = ffibinding.handle_error(ffi.C.packet_inject),
	send = raw_dissector.method.send,
	receive = raw_dissector.method.receive,
	continue = haka.helper.Dissector.method.continue,
	can_continue = raw_dissector.method.can_continue,
	error = ffibinding.handle_error(ffi.C.packet_drop),
	name = "raw",
	issent = function(pkt) return ffibinding.handle_error(ffi.C.packet_state)(pkt) == "sent" end,
}

ffibinding.set_meta("struct packet", prop, meth, {})

local prop = {
	seconds = ffi.C.time_sec,
}

local mt = {
	__tostring = function (self)
		local res = ffi.new[[
			char[27] /* \see TIME_BUFSIZE */
		]]
		if not ffi.C.time_tostring(self, res, 27) then
			return nil
		end

		return ffi.string(res)
	end,
}
ffibinding.set_meta("struct time", prop, {}, mt)

packet_new = ffibinding.handle_error(ffi.C.packet_new)

#endif

function haka.filter(pkt)
	raw_dissector:receive(pkt)
end

function module.create(size)
	return raw_dissector:create(size)
end

module.events = raw_dissector.events
module.options = raw_dissector.options

return module

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <haka/config.h>

local module = {}

#ifdef HAKA_FFI

local ffibinding = require('ffibinding')
local ffi = require('ffi')
ffi.cdef[[
	void packet_receive_wrapper_wrap(struct packet_object *pkt, void *state, struct receive_result *result);
	void packet_drop(struct packet *pkt);

	struct receive_result {
		bool has_extra;
		bool stop;
	};
]]

local res = ffi.new[[
	struct receive_result[1]
]]

local receive = ffibinding.object_wrapper('struct packet')(ffi.C.packet_receive_wrapper_wrap)

local ffi_state

function module.prepare(state)
	ffi_state = ffi.cast("void *", state)
end

function module.receive(state)
	local pkt = receive(ffi_state, res)

	if pkt ~= nil then ffibinding.own(pkt)
	else pkt = nil end

	return pkt, res[0].has_extra, res[0].stop
end

function module.error(pkt)
	C.packet_drop(pkt)
end

#else

module.receive, module.error = unpack({...})

#endif

function module.run(state, run_extra)
	local filter = haka.filter
	local pkt, has_extra, stop

	if module.prepare then
		module.prepare(state)
	end

	while true do
		pkt, has_extra, stop = module.receive(state)
		if has_extra then
			run_extra(state)
		end
		if stop then
			break
		end
		if pkt ~= nil then
			local ret = pcall(filter, pkt)
			if not ret then
				module.error(pkt)
			end
		end
	end
end

return module

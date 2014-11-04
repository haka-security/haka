-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <haka/config.h>

local module = {}

#ifdef HAKA_FFI

local ffi = require('ffi')
ffi.cdef[[
	int packet_receive_wrapper_wrap(void *state, struct receive_result *result);
	void packet_drop(struct packet *pkt);
	void packet_release(struct packet *pkt);

	struct receive_result {
		struct packet *pkt;
		bool has_extra;
		bool stop;
	};
]]

local res = ffi.new[[
	struct receive_result[1]
]]
local C = ffi.C

function module.receive(_state)
	local state = ffi.cast("void *", _state)
	C.packet_receive_wrapper_wrap(state, res)

	local pkt = res[0].pkt
	if pkt ~= nil then ffi.gc(pkt, ffi.C.packet_release)
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

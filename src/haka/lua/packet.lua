-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ffi = require('ffi')
ffi.cdef[[
	int packet_receive_wrapper_wrap(void *state, struct receive_result *result);

	struct receive_result {
		struct packet *pkt;
		bool has_interrupts;
		bool stop;
	};
]]

local state = ffi.cast("void *", _state)

local res = ffi.new[[
	struct receive_result[1]
]]
local C = ffi.C

return function(_state)
	local state = ffi.cast("void *", _state)
	C.packet_receive_wrapper_wrap(state, res)
	return res[0].pkt, res[0].has_interrupts, res[0].stop
end

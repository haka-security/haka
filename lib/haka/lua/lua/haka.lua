-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ffi = require('ffi')
local ffibinding = require('ffibinding')
ffi.cdef[[
	struct time {
	};

	bool time_tostring(const struct time *t, char *buffer, size_t len);
	double time_sec(const struct time *t);
]]

local prop = {
	seconds = ffi.C.time_sec,
}

local meth = {
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

ffibinding.set_meta("struct time", prop, meth, mt)

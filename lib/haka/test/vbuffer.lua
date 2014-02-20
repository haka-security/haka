-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local function test_invalid_iterator()
	local buf = haka.vbuffer("toto")
	local pos = buf:pos(0)

	pos:advance(1)

	buf:sub(0, 2):erase()

	local success, msg = pcall(function () pos:advance(1) end)
	assert(not success and msg == "invalid buffer iterator", "failed to detect invalid iterator")
end

test_invalid_iterator()

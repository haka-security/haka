-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local function test_invalid_iterator_erase_from_start()
	local buf = haka.vbuffer("toto")
	local pos = buf:pos(0)

	pos:advance(1)

	buf:sub(0, 2):erase()

	local success, msg = pcall(function () pos:advance(1) end)
	assert(not success and msg == "invalid buffer iterator", "failed to detect invalid iterator")
end

local function test_invalid_iterator_erase()
	local buf = haka.vbuffer("toto")
	local pos = buf:pos(0)

	pos:advance(2)

	buf:sub(1, 3):erase()

	local success, msg = pcall(function () pos:advance(1) end)
	assert(not success and msg == "invalid buffer iterator", "failed to detect invalid iterator")
end

local function test_asbit_empty()
	local buf = haka.vbuffer(0)
	local success, msg = pcall(function () buf:sub():asbits(8, 6) end)
	assert(not success and msg == "asbits: invalid bit offset", "failed to detect invalid asbits offset")
end

local function test_setbit_empty()
	local buf = haka.vbuffer(0)
	local success, msg = pcall(function () buf:sub():setbits(8, 6, 0) end)
	assert(not success and msg == "setbits: invalid bit offset", "failed to detect invalid setbits offset")
end

local function test_asbit_too_small()
	local buf = haka.vbuffer(2)
	local success, msg = pcall(function () buf:sub():asbits(12, 24) end)
	assert(not success and msg == "asbits: invalid bit size", "failed to detect invalid asbits size")
end

local function test_setbit_too_small()
	local buf = haka.vbuffer(2)
	local success, msg = pcall(function () buf:sub():setbits(12, 24, 0) end)
	assert(not success and msg == "setbits: invalid bit size", "failed to detect invalid setbits size")
end

test_invalid_iterator_erase_from_start()
test_invalid_iterator_erase()
test_asbit_empty()
test_setbit_empty()
test_asbit_too_small()
test_setbit_too_small()

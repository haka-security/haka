-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require('luaunit')

TestVBuffer = {}

function TestVBuffer:test_invalid_iterator_erase_from_start()
	local buf = haka.vbuffer_from("toto")
	local pos = buf:pos(0)

	pos:advance(1)

	buf:sub(0, 2):erase()

	local success, msg = pcall(function () pos:advance(1) end)
	assertEquals(not success and msg, "invalid buffer iterator")
end

function TestVBuffer:test_invalid_iterator_erase()
	local buf = haka.vbuffer_from("toto")
	local pos = buf:pos(0)

	pos:advance(2)

	buf:sub(1, 3):erase()

	local success, msg = pcall(function () pos:advance(1) end)
	assertEquals(not success and msg, "invalid buffer iterator")
end

function TestVBuffer:test_asbit_empty()
	local buf = haka.vbuffer_allocate(0)
	local success, msg = pcall(function () buf:sub():asbits(8, 6) end)
	assertEquals(not success and msg, "asbits: invalid bit offset")
end

function TestVBuffer:test_setbit_empty()
	local buf = haka.vbuffer_allocate(0)
	local success, msg = pcall(function () buf:sub():setbits(8, 6, 0) end)
	assertEquals(not success and msg, "setbits: invalid bit offset")
end

function TestVBuffer:test_asbit_too_small()
	local buf = haka.vbuffer_allocate(2)
	local success, msg = pcall(function () buf:sub():asbits(12, 24) end)
	assertEquals(not success and msg, "asbits: invalid bit size")
end

function TestVBuffer:test_asnumber_too_big()
	local buf = haka.vbuffer_allocate(10)
	local success, msg = pcall(function () buf:sub():asnumber() end)
	assertEquals(not success and msg, "asnumber: unsupported size 10")
end

function TestVBuffer:test_setbit_too_big()
	local buf = haka.vbuffer_allocate(8)
	local success, msg = pcall(function () buf:sub():setbits(12, 2000, 2000) end)
	assertEquals(not success and msg, "setbits: unsupported size 2000")
end

function TestVBuffer:test_setbit_too_small()
	local buf = haka.vbuffer_allocate(2)
	local success, msg = pcall(function () buf:sub():setbits(12, 24, 0) end)
	assertEquals(not success and msg, "setbits: invalid bit size")
end

function TestVBuffer:test_insert_on_itself()
	local buf = haka.vbuffer_allocate(10)
	local success, msg = pcall(function () buf:pos(4):insert(buf) end)
	assertEquals(not success and msg, "circular buffer insertion")
end

function TestVBuffer:test_append_on_itself()
	local buf = haka.vbuffer_allocate(10)
	local success, msg = pcall(function () buf:append(buf) end)
	assertEquals(not success and msg, "circular buffer insertion")
end

LuaUnit:setVerbosity(1)
assert(LuaUnit:run('TestVBuffer') == 0)

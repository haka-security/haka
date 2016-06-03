-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

TestVBufferStream = {}

function TestVBufferStream:gen_stream(f)
	local stream = haka.vbuffer_stream()
	local manager = haka.vbuffer_stream_comanager:new()
	manager:start(0, f)

	for i=10,1,-1 do
		local current = stream:push(haka.vbuffer_from("Haka"))
		if i == 1 then
			stream:finish()
		end

		manager:process_all(current)
	end
end

function TestVBufferStream:test_stream_blocking_advance()
	local count = 0
	local loop = 0

	self:gen_stream(function (iter)
		while not iter.iseof do
			count = count + iter:advance(10)
			loop = loop+1
		end
	end)

	assertEquals(count, 10*4) -- 10 time "Haka"
	assertEquals(loop, 5)
end

function TestVBufferStream:test_stream_blocking_sub()
	local loop = 0
	local ref = { "HakaHakaHa", "kaHakaHaka", "HakaHakaHa", "kaHakaHaka", "" }

	self:gen_stream(function (iter)
		while true do
			local sub = iter:sub(10)
			if not sub then break end

			assertEquals(sub:asstring(), ref[loop+1])
			loop = loop+1
		end
	end)

	assertEquals(loop, 4)
end

function TestVBufferStream:test_stream_blocking_all()
	self:gen_stream(function (iter)
		assertEquals(iter:sub('all'):asstring(), "HakaHakaHakaHakaHakaHakaHakaHakaHakaHaka")
	end)
end

function TestVBufferStream:test_stream_blocking_available()
	local loop = 0

	self:gen_stream(function (iter)
		for sub in iter:foreach_available() do
			assertEquals(sub:asstring(), "Haka")
			loop = loop+1
		end
	end)

	assertEquals(loop, 10)
end

addTestSuite('TestVBufferStream')

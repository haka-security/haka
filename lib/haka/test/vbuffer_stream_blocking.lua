-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local function gen_stream(f)
	local stream = haka.vbuffer_stream()
	local manager = haka.vbuffer_stream_comanager:new(stream)
	manager:start(0, f)

	for i=10,1,-1 do
		stream:push(haka.vbuffer("Haka"))
		if i == 1 then
			stream:finish()
		end

		manager:process_all()
	end
end

local function test_stream_blocking_advance()
	local count = 0
	local loop = 0

	gen_stream(function (iter)
		while not iter.isend do
			count = count + iter:advance(10)
			loop = loop+1
		end
	end)

	assert(count == 10*4, string.format("invalid byte count: %i", count)) -- 10 time "Haka"
	assert(loop == 5, string.format("invalid loop count: %i", loop))
end

local function test_stream_blocking_sub()
	local loop = 0
	local ref = { "HakaHakaHa", "kaHakaHaka", "HakaHakaHa", "kaHakaHaka", "" }

	gen_stream(function (iter)
		while true do
			local sub = iter:sub(10)
			if not sub then break end

			assert(sub:asstring() == ref[loop+1])
			loop = loop+1
		end
	end)

	assert(loop == 4, string.format("invalid loop count: %i", loop))
end

local function test_stream_blocking_all()
	gen_stream(function (iter)
		assert(iter:sub('all'):asstring() == "HakaHakaHakaHakaHakaHakaHakaHakaHakaHaka")
	end)
end

local function test_stream_blocking_available()
	local loop = 0

	gen_stream(function (iter)
		while true do
			local sub = iter:sub('available')
			if not sub then break end

			assert(sub:asstring() == "Haka")
			loop = loop+1
		end
	end)

	assert(loop == 10, string.format("invalid loop count: %i", loop))
end

test_stream_blocking_advance()
test_stream_blocking_sub()
test_stream_blocking_all()
test_stream_blocking_available()

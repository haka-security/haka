-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

package.path = string.gsub('@CONF@', '[^/]*%.lua', '') .. '?.lua;' .. package.path

require('luaunit')

-- Add extra utilities

local luaunit_tests = {}

function assertError(msgre, f, ...)
	local success, error_msg = pcall(f, ...)
	if not success then
		if not string.match(error_msg, msgre) then
			error(string.format("Error '%s' does not match expected '%s'", error_msg, msgre), 2)
		end

		return
	end

	error("Expected an error but no error generated", 2)
end

function addTestSuite(...)
	for _,test in ipairs{...} do
		table.insert(luaunit_tests, test)
	end
end

-- Run the Lua code

print("UNIT TEST BEGIN")

local function call()
	require(string.gsub(string.match('@CONF@', '[^/]*%.lua'), '%.lua', ''))
end

local success, msg = pcall(call)
if not success then
	print("UNIT TEST FAILED")
	print(msg)
	error("Test failed")
end

LuaUnit:setVerbosity(2)

for _, test in ipairs(luaunit_tests) do
	if LuaUnit:run(test) ~= 0 then
		print(string.format("LUAUNIT TEST '%s' FAILED", test))
		error("Test failed")
	end
end

print("UNIT TEST END")

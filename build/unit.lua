-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

package.path = '?.lua;' .. package.path

print("UNIT TEST BEGIN")

local function call()
	require(string.gsub('@CONF@', '.lua', ''))
end

local success, msg = pcall(call)

if success then
	print("UNIT TEST END")
else
	print("UNIT TEST FAILED")
	print(msg)
	error("Test failed")
end

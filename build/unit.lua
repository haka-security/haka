
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
end

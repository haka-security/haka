-- Basic test that will output some basic information about the
-- received packets.

function bool(val)
	if(val) then
		return 1, "Set"
	end
	return 0, "Not set"
end

function checks(proto)
	if proto:verifyChecksum() then
		good = "[Good: True]"
		bad = "[Bad: False]"
	else
		good = "[Good: False]"
		bad = "[Bad: True]"
	end
return good, bad
end 

require("ipv4")
require("tcp")

haka2.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		pkt:computeChecksum()
	end
}

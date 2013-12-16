------------------------------------
-- Loading dissectors
------------------------------------

-- Load the ipv4 dissector to filter on IPV4 fields
local ipv4 = require('protocol/ipv4')

------------------------------------
-- Detects packets originating from 192.168.10.10
--     * Log an alert
--     * Drop the packet
------------------------------------
haka.rule{
	-- This rule is applied on each IP incoming packet
	hooks = { 'ipv4-up' },
	eval = function (self, pkt)
		-- Parse the IP address and assign it to a variable
		local bad_ip = ipv4.addr('192.168.10.10')
		if pkt.src == bad_ip then
			-- Log an alert
			haka.alert{
				description = string.format("Filtering IP %s", bad_ip),
				severity = 'low'
			}
			-- and drop the packet
			pkt:drop()
		end
		-- All other packets will be accepted
	end
}

------------------------------------
-- Loading dissectors
------------------------------------
local ipv4 = require('protocol/ipv4')

-- drop all packets originating from
-- blacklisted address 192.168.10.10
haka.rule{
	-- This rule is applied on each ip incoming packet
	on = haka.dissectors.ipv4.events.receive_packet,
	eval = function (pkt)
		-- Parse the IP address and assign it to a variable
		local bad_ip = ipv4.addr('192.168.10.10')
		if pkt.src == bad_ip then
			-- Raise an alert
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

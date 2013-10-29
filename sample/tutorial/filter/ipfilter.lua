------------------------------------
-- Loading dissectors
------------------------------------

-- We assign ipv4 because we'll need some function
-- provided by dissector later in a security rule
local ipv4 = require('protocol/ipv4')

------------------------------------
-- Security rule
------------------------------------
haka.rule {
        -- This rule is applied on each IP incoming packet
        hooks = {'ipv4-up'},
        eval = function (self, pkt)
			-- We assign a variable to an IP address thanks to 
			-- the ipv4.addr function
			local bad_ip = ipv4.addr('192.168.10.10')
			if pkt.src == bad_ip then
				-- We want to block this IP
				pkt:drop()
				haka.log.info("Filter", "Filtering IP 192.168.10.10")
			end
			-- All other packets will be accepted
        end
}

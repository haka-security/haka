------------------------------------
-- Loading dissectors
------------------------------------
require('protocol/ipv4')
require('protocol/tcp')
local tcp_connection = require('protocol/tcp_connection')

------------------------------------
-- Security rule group
------------------------------------

-- Create a new group with no rules in it
-- The 'my_group' lua variable will be used
-- to add rules to the group
local my_group = haka.rule_group{
	name = "my_group",
	hook = tcp_connection.events.new_connection,
	init = function (flow, pkt)
		haka.log.debug("Entering packet filtering rules : %d --> %d",
			pkt.srcport, pkt.dstport)
	end,

	-- rules will return a boolean
	-- if the boolean is false, we continue
	-- if the boolean is true, we leave the group immediately
	continue = function (ret, flow, pkt)
		return not ret
	end,

	-- if we reach the final function, all rules have returned false
	-- Nobody has accepted the packet => we drop it.
	final = function (flow, pkt)
		haka.alert{
			description = "Packet dropped : drop by default",
			targets = { haka.alert.service("tcp", pkt.dstport) }
		}
		pkt:drop()
	end
}

------------------------------------
-- Security rules for my_group
------------------------------------

my_group:rule{
	eval = function (flow, pkt)
		if pkt.dstport == 80 then
			haka.log("Authorizing traffic on port 80")
			return true
		end
	end
}

my_group:rule{
	eval = function (flow, pkt)
		if pkt.dstport == 22 then
			haka.log("Authorizing traffic on port 22")
			return true
		end
	end
}

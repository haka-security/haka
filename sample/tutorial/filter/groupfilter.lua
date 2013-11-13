------------------------------------
-- Loading dissectors
------------------------------------

require('protocol/ipv4')
require('protocol/tcp')
require('protocol/tcp-connection')

------------------------------------
-- Security group
------------------------------------

-- Here, we create a group rule
-- A group have an init function (self explanatory)
-- then a continue function wich tells what to do after each
-- rule. As written here, it says that if a rule return true, it will 
-- stop the evaluation of other rules. If false, it continues to the next rule.
-- The fini function is the last evaluation made by the group.
-- For this test, it permits us to do a "Default drop" with haka and
-- authorize only connection to specific ports (ssh and http)

local group = haka.rule_group {
	-- The hooks tells where this rule is applied
	-- only TCP packets will be concerned by this rule
	-- other protocol will flow
	hook = haka.event('tcp-connection', 'new_connection'),
	init = function (flow, pkt)
		haka.log.debug("filter", "Entering packet filtering rules : %d --> %d", pkt.srcport, pkt.dstport)
	end,
	fini = function (flow, pkt)
		haka.alert{
			description = "Packet dropped : drop by default",
			targets = { haka.alert.service("tcp", pkt.dstport) }
		}
		pkt:drop()
	end,
	continue = function (ret)
		return not ret
	end
}

------------------------------------
-- Security group rule
------------------------------------
group:rule(
	function (flow, pkt)
		-- Accept connection to TCP port 80
		if pkt.dstport == 80 then
			haka.log("Filter", "Authorizing traffic on port 80")
			return true
		end
	end
)

group:rule(
	function (flow, pkt)
		-- Accept connection to TCP port 22
		if pkt.dstport == 22 then
			haka.log("Filter", "Authorizing traffic on port 22")
			return true
		end
	end
)

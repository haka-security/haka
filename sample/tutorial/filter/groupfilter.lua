------------------------------------
-- Loading dissectors
------------------------------------

require('protocol/ipv4')
require('protocol/tcp')

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
	name = "group",
	init = function (self, pkt)
		haka.log.debug("filter", "Entering packet filtering rules : %d --> %d", pkt.tcp.srcport, pkt.tcp.dstport)
	end,
	fini = function (self, pkt)
		haka.log.error("filter", "Packet dropped : drop by default")
		pkt:drop()
	end,
	continue = function (self, pkt, ret)
		return not ret
	end
}

------------------------------------
-- Security group rule
------------------------------------
group:rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		-- Accept connection to TCP port 80
		if pkt.dstport == 80 then
			haka.log("Filter", "Authorizing traffic on port 80")
			return true
		end
	end
}

group:rule {
	hooks = { 'tcp-connection-new' },
	eval = function (self, pkt)
		-- Accept connection to TCP port 22
		if pkt.dstport == 22 then
			haka.log("Filter", "Authorizing traffic on port 22")
			return true
		end
	end
}

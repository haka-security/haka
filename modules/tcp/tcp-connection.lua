module("tcp-connection", package.seeall)

haka2.dissector {
	name = "tcp-connection",
	dissect = function (pkt)

		local newpkt = {}
		newpkt.__tcp = pkt
		newpkt.connection = pkt:getConnection()
		newpkt.dissector = "tcp-connection"
		newpkt.nextDissector = nil
		newpkt.valid = function (self)
			return newpkt.connection ~= nil
		end
		newpkt.drop = function (self)
			newpkt.connection:close()
			newpkt.connection = nil
		end
		newpkt.forge = function (self)
			local tcp = self.__tcp
			self.__tcp = nil
			return tcp
		end

		if not newpkt.connection then
			-- new connection
			if pkt.flags.syn then
				if haka2.rule_hook("tcp-connection-new", pkt) then
					return nil
				end

				newpkt.connection = pkt:newConnection()
				haka.log.debug("tcp-connection", "new connection %s (%d) -> %s (%d)", newpkt.connection.srcip,
					newpkt.connection.srcport, newpkt.connection.dstip, newpkt.connection.dstport)
			else
				haka.log.error("tcp-connection", "no connection found")
				pkt:drop()
				return nil
			end
		else
			-- end existing connection
			if pkt.flags.fin or pkt.flags.rst then
				haka.log.debug("tcp-connection", "ending connection %s (%d) -> %s (%d)", newpkt.connection.srcip,
					newpkt.connection.srcport, newpkt.connection.dstip, newpkt.connection.dstport)
				newpkt.connection:close()
			end
		end

		return newpkt
	end
}

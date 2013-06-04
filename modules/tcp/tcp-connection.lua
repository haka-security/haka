module("tcp-connection", package.seeall)

local function forge(self)
	pkt = self.stream:pop()
	if pkt then
		self.connection:stream(not self.direction):ack(pkt)

			haka.log.debug("tcp-connection", "send %s:%d-%d -> %s:%d-%d [len=%d]", pkt.ip.src,
				pkt.srcport, pkt.seq, pkt.ip.dst, pkt.dstport, pkt.ack_seq, #pkt.payload)
			
			return pkt
	end
end

local function drop(self)
	self.connection:close()
	self.connection = nil
end

-- TODO
local __tcp = {}

local function dissect(pkt)
	local stream_dir

	haka.log.debug("tcp-connection", "received %s:%d-%d -> %s:%d-%d [len=%d]", pkt.ip.src,
				pkt.srcport, pkt.seq, pkt.ip.dst, pkt.dstport, pkt.ack_seq, #pkt.payload)

	local newpkt = {}
	newpkt.connection, stream_dir = pkt:getConnection()
	newpkt.dissector = "tcp-connection"
	newpkt.nextDissector = nil
	newpkt.valid = function (self)
		return newpkt.connection ~= nil
	end
	newpkt.drop = drop
	newpkt.forge = forge

	table.insert(__tcp, pkt)

	if not newpkt.connection then
		-- new connection
		if pkt.flags.syn then
			if haka.rule_hook("tcp-connection-new", pkt) then
				return nil
			end

			newpkt.connection = pkt:newConnection()
			stream_dir = true
			haka.log.debug("tcp-connection", "new connection %s (%d) -> %s (%d)", newpkt.connection.srcip,
				newpkt.connection.srcport, newpkt.connection.dstip, newpkt.connection.dstport)
		else
			haka.log.error("tcp-connection", "no connection found")
			pkt:drop()
			return nil
		end
	end

	newpkt.stream = newpkt.connection:stream(stream_dir)
	newpkt.stream:push(pkt)
	newpkt.direction = stream_dir

	haka.log.debug("tcp-connection", "stream %s - %u - len=%d - avail=%d", stream_dir, pkt.seq,
		#pkt.payload, newpkt.stream:available())

	if pkt.flags.ack and newpkt.connection.state > 0 then
		newpkt.connection.state =  newpkt.connection.state + 1
	end

	if pkt.flags.fin then
		newpkt.connection.state =  newpkt.connection.state + 1
	end
	
	if pkt.flags.syn then
		return nil
	end

	if newpkt.connection.state == 4 or pkt.flags.rst then
		haka.log.debug("tcp-connection", "ending connection %s (%d) -> %s (%d)", newpkt.connection.srcip,
			newpkt.connection.srcport, newpkt.connection.dstip, newpkt.connection.dstport)
		newpkt.connection:close()
		return nil
	end
			
	if pkt.flags.fin or pkt.flags.rst then
		haka.log.debug("tcp-connection", "ending connection %s (%d) -> %s (%d)", newpkt.connection.srcip,
			newpkt.connection.srcport, newpkt.connection.dstip, newpkt.connection.dstport)
		newpkt.connection:close()
		return nil
	end

	return newpkt
end

haka.dissector {
	name = "tcp-connection",
	dissect = dissect
}

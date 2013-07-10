module("tcp-connection", package.seeall)

local function forge(self)
	if not self.connection then
		return nil
	end

	pkt = self.stream:pop()
	if not pkt then
		pkt = table.remove(self.connection.data._queue)
		if pkt then
			self.connection:stream(self.direction):seq(pkt)
		end
	end
	
	if pkt then
		self.connection:stream(not self.direction):ack(pkt)
	end

	if self.__drop then
		self.connection:drop()
		self.connection = nil
	elseif self.__close then
		self.connection:close()
		self.connection = nil
	end

	return pkt
end

local function drop(self)
	self.__drop = true
end

local function dissect(pkt)
	local stream_dir, dropped

	local newpkt = {}
	newpkt.connection, newpkt.direction, dropped = pkt:getconnection()
	newpkt.dissector = "tcp-connection"
	newpkt.next_dissector = nil
	newpkt.valid = function (self)
		return not self.__close and not self.__drop
	end
	newpkt.drop = drop
	newpkt.forge = forge
	newpkt.__close = false
	newpkt.__drop = false

	if not newpkt.connection then
		-- new connection
		if pkt.flags.syn and not pkt.flags.ack then
			newpkt.tcp = pkt
			newpkt.connection = pkt:newconnection()
			newpkt.connection.data = {}

			haka.rule_hook("tcp-connection-new", newpkt)
			if not newpkt:valid() then
				newpkt.connection:drop()
				newpkt.connection = nil
				pkt:drop()
				return nil
			end

			newpkt.connection.data._next_dissector = newpkt.next_dissector
			newpkt.connection.data._state = 0
			newpkt.connection.data._queue = {}
			newpkt.direction = true
		else
			if not dropped then
				haka.log.error("tcp-connection", "no connection found")
				haka.log.debug("tcp-connection", "no connection found %s:%d -> %s:%d", pkt.ip.src,
					pkt.srcport, pkt.ip.dst, pkt.dstport)
			end
			pkt:drop()
			return nil
		end
	end

	newpkt.stream = newpkt.connection:stream(newpkt.direction)

	if pkt.flags.syn then
		newpkt.stream:init(pkt.seq+1)
		return nil
	elseif pkt.flags.rst then
		newpkt.__drop = true
		table.insert(newpkt.connection.data._queue, pkt)
		return newpkt
	elseif newpkt.connection.data._state >= 2 then
		if pkt.flags.ack then
			newpkt.connection.data._state = newpkt.connection.data._state + 1
		end
	
		if newpkt.connection.data._state >= 3 then
			newpkt.__close = true
		end

		table.insert(newpkt.connection.data._queue, pkt)
		return newpkt
	elseif pkt.flags.fin then
		newpkt.connection.data._state = newpkt.connection.data._state+1
		table.insert(newpkt.connection.data._queue, pkt)
		return newpkt
	else
		newpkt.stream:push(pkt)
		newpkt.next_dissector = newpkt.connection.data._next_dissector
		return newpkt
	end
end

haka.dissector {
	name = "tcp-connection",
	hooks = { "tcp-connection-new" },
	dissect = dissect
}

module("tcp-connection", package.seeall)

local function forge(self)
	if not self.connection then
		return nil
	end

	pkt = self.stream:pop()
	if not pkt then
		pkt = table.remove(self.connection.data.queue)
		if pkt then
			self.connection:stream(self.direction):seq(pkt)
		end
	end
	
	if pkt then
		self.connection:stream(not self.direction):ack(pkt)
	end

	if self.close then
		self.connection:close()
		self.connection = nil
	end

	return pkt
end

local function drop(self)
	self.close = true
end

local function dissect(pkt)
	local stream_dir

	local newpkt = {}
	newpkt.connection, newpkt.direction = pkt:getconnection()
	newpkt.dissector = "tcp-connection"
	newpkt.next_dissector = nil
	newpkt.valid = function (self)
		return not self.close
	end
	newpkt.drop = drop
	newpkt.forge = forge
	newpkt.close = false

	if not newpkt.connection then
		-- new connection
		if pkt.flags.syn then
			newpkt.tcp = pkt

			haka.rule_hook("tcp-connection-new", newpkt)
			if not newpkt:valid() then
				pkt:drop()
				return nil
			end

			newpkt.connection = pkt:newconnection()
			newpkt.connection.data = {}
			newpkt.connection.data.next_dissector = newpkt.next_dissector
			newpkt.connection.data.state = 0
			newpkt.connection.data.queue = {}
			newpkt.direction = true
		else
			haka.log.error("tcp-connection", "no connection found")
			pkt:drop()
			return nil
		end
	end

	newpkt.stream = newpkt.connection:stream(newpkt.direction)

	if pkt.flags.syn then
		newpkt.stream:init(pkt.seq+1)
		return nil
	elseif pkt.flags.rst then
		newpkt.close = true
		table.insert(newpkt.connection.data.queue, pkt)
		return newpkt
	elseif newpkt.connection.data.state >= 2 then
		if pkt.flags.ack then
			newpkt.connection.data.state = newpkt.connection.data.state + 1
		end
	
		if newpkt.connection.data.state >= 3 then
			newpkt.close = true
		end

		table.insert(newpkt.connection.data.queue, pkt)
		return newpkt
	elseif pkt.flags.fin then
		newpkt.connection.data.state = newpkt.connection.data.state+1
		table.insert(newpkt.connection.data.queue, pkt)
		return newpkt
	else
		newpkt.stream:push(pkt)
		newpkt.next_dissector = newpkt.connection.data.next_dissector
		return newpkt
	end
end

haka.dissector {
	name = "tcp-connection",
	dissect = dissect
}

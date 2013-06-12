module("tcp-connection", package.seeall)

local function forge(self)
	pkt = self.stream:pop()
	if pkt then
		self.connection:stream(not self.direction):ack(pkt)
		return pkt
	end

	if self.close then
		self.connection:close()
		self.connection = nil
	end
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
		return not newpkt.close
	end
	newpkt.drop = drop
	newpkt.forge = forge
	newpkt.close = false

	if not newpkt.connection then
		-- new connection
		if pkt.flags.syn then
			newpkt.tcp = pkt

			haka.rule_hook("tcp-connection-new", newpkt)
			if not pkt:valid() then
				return nil
			end

			newpkt.connection = pkt:newconnection()
			newpkt.connection.data = {}
			newpkt.connection.data.next_dissector = newpkt.next_dissector
			newpkt.connection.data.state = 0
			newpkt.direction = true
		else
			haka.log.error("tcp-connection", "no connection found")
			pkt:drop()
			return nil
		end
	end

	newpkt.stream = newpkt.connection:stream(newpkt.direction)

	if pkt.flags.syn then
		newpkt.stream:push(pkt)
		return nil
	end

	-- handle ending handshake
	if pkt.flags.ack and newpkt.connection.data.state > 0 then
		newpkt.connection.data.state =  newpkt.connection.data.state + 1
	end

	if pkt.flags.fin then
		newpkt.connection.data.state =  newpkt.connection.data.state + 1
	end

	if newpkt.connection.data.state == 4 or pkt.flags.rst then
		newpkt.close = true
	end

	newpkt.stream:push(pkt)
	newpkt.next_dissector = newpkt.connection.data.next_dissector

	return newpkt
end

haka.dissector {
	name = "tcp-connection",
	dissect = dissect
}

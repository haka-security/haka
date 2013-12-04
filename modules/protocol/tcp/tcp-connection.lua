-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local ipv4 = require("protocol/ipv4")
local tcp = tcp

local function forge_reset(conn, inv)
	tcprst = haka.packet.new()
	tcprst = ipv4.create(tcprst)

	if inv then
		tcprst.src = conn.dstip
		tcprst.dst = conn.srcip
	else
		tcprst.src = conn.srcip
		tcprst.dst = conn.dstip
	end

	tcprst.ttl = 64

	tcprst = tcp.create(tcprst)

	if inv then
		tcprst.srcport = conn.dstport
		tcprst.dstport = conn.srcport
	else
		tcprst.srcport = conn.srcport
		tcprst.dstport = conn.dstport
	end

	tcprst.seq = conn:stream(not inv).lastseq

	tcprst.flags.rst = true

	return tcprst
end

local function forge(self)
	if not self.connection then
		return nil
	end

	if self.__reset ~= nil then
		local pkt = forge_reset(self.connection, self.__reset==1)
		self.__reset = self.__reset-1

		if self.__reset == 0 then
			self.connection:drop()
			self.connection = nil
		end

		return pkt
	else
		local pkt = self.stream:pop()
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
			if self.__droppkt and pkt then
				pkt:drop()
			end

			self.connection:drop()
			self.connection = nil
		elseif self.__close then
			self.connection:close()
			self.connection = nil
		end

		return pkt
	end
end

local function drop(self)
	self.__drop = true
	self.__droppkt = true
end

local function reset(self)
	self.__reset = 2
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
	newpkt.reset = reset

	if not newpkt.connection then
		-- new connection
		if pkt.flags.syn and not pkt.flags.ack then
			newpkt.tcp = pkt
			newpkt.connection = pkt:newconnection()
			newpkt.connection.data = {}

			if not haka.rule_hook("tcp-connection-new", newpkt) then
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
				haka.alert{
					severity = 'low',
					description = "no connection found for tcp packet",
					sources = {
						haka.alert.address(pkt.ip.src),
						haka.alert.service(string.format("tcp/%d", pkt.srcport))
					},
					targets = {
						haka.alert.address(pkt.ip.dst),
						haka.alert.service(string.format("tcp/%d", pkt.dstport))
					}
				}
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

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local check = require('check')

local type = {}
local dissector = {}


--
-- Dissector base class
--

type.Dissector = class.class('Dissector')

function type.Dissector.__class_init(self, cls)
	self.super:__class_init(cls)
	cls.events = {}
	self.inherit_events(cls)
	cls.options = {}
end

function type.Dissector.register_event(cls, name, continue, signal, options)
	continue = continue or function (self) return self:continue() end
	cls.events[name] = haka.event.Event:new(string.format('%s:%s', cls.name, name), continue, signal, options)
end

function type.Dissector.inherit_events(cls)
	local parent_events = cls.super.events
	if parent_events then
		local events = cls.events
		for name, event in pairs(parent_events) do
			events[name] = event:clone()
			events[name].name = string.format('%s:%s', cls.name, name)
		end
	end
end

type.Dissector.auto_state_machine = true

function type.Dissector.method:__init()
	local cls = class.classof(self)
	if cls.state_machine and cls.auto_state_machine then
		self.state = cls.state_machine:instanciate(self)
	end
end

type.Dissector.property.name = {
	get = function (self) return class.classof(self).name end
}

function type.Dissector.method:trigger(signal, ...)
	haka.context:signal(self, class.classof(self).events[signal], ...)
end

function type.Dissector.method:send()
	error("not implemented")
end

function type.Dissector.method:drop()
	error("not implemented")
end

function type.Dissector.method:error()
	self:drop()
end

function dissector.pcall(self, f)
	local ret, err = xpcall(f, debug.format_error)

	if not ret then
		if err then
			haka.log.error(self.name, "%s", err)
			return self:error()
		end
	end

	-- return the result of f
	return err
end


--
-- Packet based type
--

type.PacketDissector = class.class('PacketDissector', type.Dissector)

type.PacketDissector:register_event('receive_packet')
type.PacketDissector:register_event('send_packet')

function type.PacketDissector:receive(pkt)
	local npkt = self:new(pkt)
	if not npkt then
		return
	end

	return dissector.pcall(npkt, function () npkt:receive() return npkt end)
end

function type.PacketDissector.method:receive()
	error("not implemented")
end

function type.PacketDissector.method:continue()
	error("not implemented")
end

function type.PacketDissector.method:send()
	error("not implemented")
end

function type.PacketDissector.method:inject()
	error("not implemented")
end

function type.PacketDissector.method:drop()
	error("not implemented")
end


type.EncapsulatedPacketDissector = class.class('EncapsulatedPacketDissector', type.PacketDissector)

function type.EncapsulatedPacketDissector.method:receive()
	self:parse(self._parent)
	return self:emit()
end

function type.EncapsulatedPacketDissector.method:__init(parent)
	class.super(type.EncapsulatedPacketDissector).__init(self)
	self._parent = parent
end

function type.EncapsulatedPacketDissector.method:parse(pkt)
	self._select, self._payload = pkt.payload:sub(0, 'all'):select()
	self:parse_payload(pkt, self._payload)
end

function type.EncapsulatedPacketDissector.method:parse_payload(pkt, payload)
	error("not implemented")
end

function type.EncapsulatedPacketDissector.method:create(init, pkt)
	self._select, self._payload = pkt.payload:sub(0, 'all'):select()
	self:create_payload(pkt, self._payload, init)
end

function type.EncapsulatedPacketDissector.method:create_payload(pkt, payload, init)
	error("not implemented")
end

function type.EncapsulatedPacketDissector.method:forge(pkt)
	self:forge_payload(pkt, self._payload)
	self._select:restore(self._payload)
	self._payload = nil
	self._select = nil
end

function type.EncapsulatedPacketDissector.method:forge_payload(pkt, payload)
	error("not implemented")
end

function type.EncapsulatedPacketDissector.method:continue()
	return self._parent:continue()
end

function type.EncapsulatedPacketDissector.method:drop()
	return self._parent:drop()
end

function type.EncapsulatedPacketDissector.method:next_dissector()
	return nil
end

function type.EncapsulatedPacketDissector.method:emit()
	self:trigger('receive_packet')

	local next_dissector = self:next_dissector()
	if next_dissector then
		return next_dissector:receive(self)
	else
		return self:send()
	end
end

function type.EncapsulatedPacketDissector.method:send()
	self:trigger('send_packet')

	self:forge(self._parent)
	self._parent:send()
	self._parent = nil
end

function type.EncapsulatedPacketDissector.method:inject()
	self:forge(self._parent)
	self._parent:inject()
	self._parent = nil
end


--
-- Flow based dissector
--

type.FlowDissector = class.class('FlowDissector', type.Dissector)

function type.FlowDissector.stream_wrapper(f, options, self, stream, current, ...)
	if options and options.streamed then
		self:streamed(stream, f, self, current, ...)
	else
		if (not current or not current:check_available(1)) and
		   not stream.isfinished then
			return
		end

		if current then
			local sub = current:copy():sub('available')
			if sub then
				f(self, sub, ...)
			end
		end
	end
end

function type.FlowDissector.register_streamed_event(cls, name, continue, options)
	type.Dissector.register_event(cls, name, continue, type.FlowDissector.stream_wrapper, options)
end

function type.FlowDissector.method:connections()
	local connections = class.classof(self).connections
	if connections then
		return haka.event.ObjectEventConnections:new(self, connections)
	end
end

function type.FlowDissector.method:send(pkt)
	pkt:send()
end

function type.FlowDissector.method:continue()
	error("not implemented")
end

function type.FlowDissector.method:streamed(stream, f, this, current, ...)
	if (not current or not current:check_available(1)) and
	   not stream.isfinished then
		return
	end

	local cur
	if current then cur = current:copy() end

	local comanager = self:get_comanager(stream, ...)

	-- unique id for the function to execute
	local id = comanager.hash(f, this)

	if not comanager:has(id) then
		local args = {...}
		comanager:start(id, function (iter) return f(this, iter, unpack(args)) end)
	end

	comanager:process(id, cur)
end

function type.FlowDissector.method:get_comanager(stream)
	if not self._costream then
		self._costream = {}
	end

	if not self._costream[stream] then
		self._costream[stream] = haka.vbuffer_stream_comanager:new(stream)
	end

	return self._costream[stream]
end

function type.FlowDissector.method:next_dissector()
	return self._next_dissector
end

function type.FlowDissector.method:select_next_dissector(dissector)
	haka.context:register_connections(dissector:connections())
	self._next_dissector = dissector
end

--
-- Utility functions
--
local dissectors = {}

function dissector.new(args)
	check.assert(args.type, string.format("no type defined for dissector '%s'", args.name))

	if haka.mode ~= 'console' then
		haka.log.info("dissector", "register new dissector '%s'", args.name)
	end

	local d = class.class(args.name, args.type)
	table.insert(dissectors, d)
	return d
end

local other_direction = {
	up = 'down',
	down = 'up'
};

function dissector.other_direction(dir)
	return other_direction[dir]
end

function haka.console.events()
	local ret = {}
	local event = {}
	for _, dissector in pairs(dissectors) do
		for _, event in pairs(dissector.events) do
			local er = { event=event.name, listener=0 }
			table.insert(ret, er)
			event[event.name] = er
		end
	end
	for event, listeners in pairs(haka.context.connections) do
		event[event.name].listener = #listeners
	end
	return ret
end


haka.dissector = dissector
table.merge(haka.helper, type)

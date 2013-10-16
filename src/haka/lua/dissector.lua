
local dissector = {}


--
-- Dissector base class
--

dissector.Dissector = class('Dissector')

function haka.event(dname, name)
	local dissec = dissector.get(dname)
	if not dissec then
		error(string.format("unknown dissector '%s'", dname))
	end

	local event = dissec.events[name]
	if not event then
		error(string.format("unknown event '%s' on dissector '%s'", name, dname))
	end
	
	return event
end

function dissector.Dissector.__class_init(self, cls)
	self.super:__class_init(cls)
	cls.events = {}
	self.inherit_events(cls)
	cls.options = {}
end

function dissector.Dissector.register_event(cls, name, continue)
	continue = continue or function (self) return self:continue() end
	cls.events[name] = haka.events.Event:new(string.format('%s:%s', cls.name, name), continue)
end

function dissector.Dissector.inherit_events(cls)
	local parent_events = cls.super.events
	if parent_events then
		local events = cls.events
		for name, event in pairs(parent_events) do
			events[name] = event:clone()
			events[name].name = string.format('%s:%s', cls.name, name)
		end
	end
end

function dissector.Dissector.method:__init()
end

function dissector.Dissector.property.get:name()
	return classof(self).name
end

function dissector.Dissector.receive(pkt)
	error("not implemented")
end


--
-- Packet base dissector
--

dissector.PacketDissector = class('PacketDissector', dissector.Dissector)

dissector.PacketDissector:register_event('receive_packet')
dissector.PacketDissector:register_event('send_packet')

function dissector.PacketDissector.method:continue()
	error("not implemented")
end

function dissector.PacketDissector.method:send()
	error("not implemented")
end


--
-- Flow base dissector
--

dissector.FlowDissector = class('FlowDissector', dissector.Dissector)

dissector.FlowDissector:register_event('receive_data')
dissector.FlowDissector:register_event('send_data')

function dissector.FlowDissector.method:connections()
	local connections = classof(self).connections
	if connections then
		return haka.events.ObjectEventConnections:new(self, connections)
	end
end

function dissector.FlowDissector.method:continue()
	error("not implemented")
end


--
-- Utility functions
--
local dissectors = {}

function dissector.new(args)
	haka.log.info("dissector", "register new dissector '%s'", args.name)
	local d = class(args.name, args.type or dissector.Dissector)
	dissectors[args.name] = d
	return d
end

function dissector.get(name)
	return dissectors[name]
end

local other_direction = {
	up = 'down',
	down = 'up'
};

function dissector.other_direction(dir)
	return other_direction[dir]
end


haka.dissector = dissector

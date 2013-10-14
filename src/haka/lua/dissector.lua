
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

dissector.PacketDissector:register_event(
	'receive_packet',
	function (self) self:continue() end
)

dissector.PacketDissector:register_event(
	'send_packet',
	function (self) self:continue() end
)

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

dissector.FlowDissector:register_event(
	'receive_data',
	function (self) return self:continue() end
)

dissector.FlowDissector:register_event(
	'send_data',
	function (self) return self:continue() end
)

function dissector.FlowDissector.method:connections()
	if self:class().connections then
		return self:class().connections
	end
end

function dissector.FlowDissector.method:continue()
	error("not implemented")
end

function dissector.FlowDissector.method:send()
	error("not implemented")
end


--
-- Statefull flow dissectors
--

dissector.StatefullFlowDissector = class('StatefullFlowDissector', dissector.FlowDissector)

function dissector.StatefullFlowDissector:__init()
	self.state = self:class().states:instanciate(self)
end

function dissector.StatefullFlowDissector:update_state(data, direction)
	if direction == 'up' then
		self.state:input(data)
	else
		self.state:output(data)
	end
end


dissector.StatefullUpDownFlowDissector = class('StatefullUpDownFlowDissector', dissector.FlowDissector)

function dissector.StatefullUpDownFlowDissector:__init()
	self.state_up = self:class().states:instanciate(self)
	self.state_down = self:class().states:instanciate(self)
end

function dissector.StatefullUpDownFlowDissector:update_state(data, direction)
	if direction == 'up' then
		self.state_up:input(data)
	else
		self.state_up:output(data)
	end
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


haka.dissector = dissector

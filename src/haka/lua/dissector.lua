
local dissector = {}


--
-- Dissector base class
--

dissector.Dissector = class('Dissector')

function events(name)
	local dissec = dissector.get(name)
	if dissec then
		return dissec.events
	end
end

function dissector.Dissector.__class_init(self, cls)
	self.super:__class_init(cls)
	cls.events = {}
	self.inherit_events(cls)
end

function dissector.Dissector.register_event(cls, name, continue)
	cls.events[name] = haka.events.Event:new(name, continue)
end

function dissector.Dissector.inherit_events(cls)
	local parent_events = cls.super.events
	if parent_events then
		local events = cls.events
		for name, event in pairs(parent_events) do
			events[name] = event:clone()
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
	'packet_received',
	function (self) self:continue() end
)

dissector.PacketDissector:register_event(
	'sending_packet',
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
	'data_available',
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
	local d = class(args.name, args.type or dissector.Dissector)
	dissectors[args.name] = d
	return d
end

function dissector.get(name)
	return dissectors[name]
end


--
-- Options
--
dissector.behavior = {}

dissector.behavior.drop_unknown_dissector = false --true


haka.dissector = dissector

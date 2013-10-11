
local events = {}


events.Event = class('Event')

function events.Event.method:__init(name, continue)
	self.name = name
	self.continue = continue
end


events.EventConnections = class('EventConnections')

function events.EventConnections.method:__init(name)
end

function events.EventConnections.method:register(event, func)
	assert(isa(event, events.Event), "event expected")
	assert(type(func) == 'function', "function expected")

	local listeners = self[event]
	if not listeners then
		listeners = {}
		self[event] = listeners
	end

	table.insert(listeners, func)
end

function events.EventConnections.method:_signal(listener, emitter, ...)
	listener(emitter, ...)
end

function events.EventConnections.method:signal(emitter, event, ...)
	local listeners = self[event]
	if listeners then
		haka.log.debug("event", "signal '%s:%s'", emitter.name, event.name)

		for _, listener in ipairs(listeners) do
			self:_signal(listener, emitter, ...)

			if not event.continue(emitter, ...) then
				return false
			end
		end
	end
	return true
end


events.ObjectEventConnections = class('ObjectEventConnections', events.EventConnections)

function events.ObjectEventConnections.method:__init(object, connections)
	self.object = object
	self.connections = connections
end

function events.ObjectEventConnections.method:_signal(listener, emitter, ...)
	listener(self.object, emitter, ...)
end

haka.events = events

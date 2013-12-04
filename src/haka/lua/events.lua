-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local events = {}


events.Event = class('Event')

function events.Event.method:__init(name, continue)
	self.name = name
	self.continue = continue
end


events.EventConnections = class('EventConnections')

function events.EventConnections.method:signal(emitter, event, ...)
	local listeners = self:_get(event)
	if listeners then
		haka.log.debug("event", "signal '%s', %d listeners", event.name, #listeners)

		for _, listener in ipairs(listeners) do
			self:_signal(listener, emitter, ...)
	
			if not event.continue(emitter) then
				return true
			end
		end
	end
	return true
end


events.StaticEventConnections = class('StaticEventConnections', events.EventConnections)

function events.StaticEventConnections.method:register(event, func)
	assert(isa(event, events.Event), "event expected")
	assert(type(func) == 'function', "function expected")

	local listeners = self[event]
	if not listeners then
		listeners = {}
		self[event] = listeners
	end

	table.insert(listeners, func)
end

function events.StaticEventConnections.method:_signal(listener, emitter, ...)
	listener(emitter, ...)
end

function events.StaticEventConnections.method:_get(event)
	return self[event]
end


events.ObjectEventConnections = class('ObjectEventConnections', events.EventConnections)

function events.ObjectEventConnections.method:__init(object, connections)
	assert(object)
	assert(connections)
	self.object = object
	self.connections = connections
end

function events.ObjectEventConnections.method:_signal(listener, emitter, ...)
	listener(self.object, emitter, ...)
end

function events.ObjectEventConnections.method:_get(event)
	return self.connections[event]
end

haka.events = events

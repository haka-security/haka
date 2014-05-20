-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local module = {}

--
-- Event
--

module.Event = class.class('Event')

function module.Event.method:__init(name, continue, signal)
	self.name = name
	self.continue = continue or function () end
	self.signal = signal or function (f, options, ...) return f(...) end
end


--
-- Connections
--

module.EventConnections = class.class('EventConnections')

function module.EventConnections.method:_signal(event, listener, emitter, ...)
	event.signal(listener.f, listener.options, emitter, ...)
end

function module.EventConnections.method:signal(emitter, event, ...)
	local listeners = self:_get(event)
	if listeners then
		haka.log.debug("event", "signal '%s', %d listeners", event.name, #listeners)

		for _, listener in ipairs(listeners) do
			self:_signal(event, listener, emitter, ...)
			event.continue(emitter, ...)
		end
	end

	return true
end


module.StaticEventConnections = class.class('StaticEventConnections', module.EventConnections)

function module.StaticEventConnections.method:register(event, func, options)
	assert(class.isa(event, module.Event), "event expected")
	assert(type(func) == 'function', "function expected")

	local listeners = self[event]
	if not listeners then
		listeners = {}
		self[event] = listeners
	end

	table.insert(listeners, {f=func, options=options})
end

function module.StaticEventConnections.method:_get(event)
	return self[event]
end


module.ObjectEventConnections = class.class('ObjectEventConnections', module.EventConnections)

module.self = {}

function module.ObjectEventConnections.method:__init(object, connections)
	assert(object)
	assert(connections)
	self.object = object
	self.connections = connections
end

function module.ObjectEventConnections.method:_signal(event, listener, emitter, ...)
	module.ObjectEventConnections.current = self.object
	event.signal(listener.f, listener.options, emitter, ...)
end

function module.ObjectEventConnections.method:_get(event)
	return self.connections[event]
end

function module.method(object, func)
	return function (...)
		if object == module.self then
			return func(module.ObjectEventConnections.current, ...)
		else
			return func(object, ...)
		end
	end
end


haka.event = module

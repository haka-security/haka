-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local Scope = class.class()

function Scope.method:__init()
	self._connections = {}
end

function Scope.method:createnamespace(ref, data)
	self[ref] = data
end

function Scope.method:namespace(ref)
	return self[ref]
end


local Context = class.class()

function Context.method:__init()
	self.connections = haka.event.StaticEventConnections:new()
end

function Context.method:signal(emitter, event, ...)
	assert(class.classof(event, haka.event.Event), "event expected")

	if not self.connections:signal(emitter, event, ...) then
		return false
	end

	if self.scope then
		for _, connections in ipairs(self.scope._connections) do
			if not connections:signal(emitter, event, ...) then
				return false
			end
		end
	end

	return true
end

function Context.method:newscope()
	return Scope:new()
end

function Context.method:exec(scope, func)
	local old = self.scope
	self.scope = scope
	local success, msg = xpcall(func, debug.format_error)
	self.scope = old
	if not success then error(msg, 0) end
end

function Context.method:register_connections(connections)
	if connections then
		if self.scope then
			table.insert(self.scope._connections, connections)
		else
			error("scope required to register dynamic event connections")
		end
	end
end


haka.context = Context:new()

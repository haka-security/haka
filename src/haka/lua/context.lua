-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local LocalContext = class()

function LocalContext.method:__init()
	self._connections = {}
	self._dissector_connections = {}
end

function LocalContext.method:createnamespace(ref, data)
	self[ref] = data
end

function LocalContext.method:namespace(ref)
	return self[ref]
end


local Context = class()

function Context.method:__init()
	self.connections = haka.events.StaticEventConnections:new()
end

function Context.method:signal(emitter, event, ...)
	if not self.connections:signal(emitter, event, ...) then
		return false
	end

	if self.context then
		for _, connections in ipairs(self.context._connections) do
			if not connections:signal(emitter, event, ...) then
				return false
			end
		end

		for _, connections in ipairs(self.context._dissector_connections) do
			if not connections:signal(emitter, event, ...) then
				return false
			end
		end
	end

	return true
end

function Context.method:newscope()
	return LocalContext:new()
end

function Context.method:scope(context, func)
	local old = self.context
	self.context = context
	local success, msg = xpcall(func, debug.format_error)
	self.context = old
	if not success then error(msg, 0) end
end

function Context.method:install_dissector(dissector)
	if self.context then
		local connections = dissector:connections()
		if connections then
			table.insert(self.context._dissector_connections, connections)
		end
	else
		error("invalid context")
	end
end


haka.context = Context:new()

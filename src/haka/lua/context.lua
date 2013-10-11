
local LocalContext = class()

function LocalContext.method:__init()
	self._connections = {}
end

function LocalContext.method:createnamespace(ref, data)
	self[ref] = data
end

function LocalContext.method:namespace(ref)
	return self[ref]
end


local Context = class()

function Context.method:__init()
	self.connections = haka.events.EventConnections:new()
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
	end

	return true
end

function Context.method:newlocal()
	return LocalContext:new()
end

function Context.method:scope(context, func)
	local old = self.context
	self.context = context
	local ret, msg = haka.pcall(func)
	self.context = old
	if not ret then error(msg) end
end

function Context.method:install_dissector(dissector)
	if self.context then
		local instance = dissector:get(self.context)
		if not instance then
			error(string.format("unknown dissector '%s'", dissector.name))
		end

		table.insert(self.context.dissectors, instance)
		
		local connections = instance:connections()
		if connections then
			table.insert(self.context.connections, connections)
		end
	else
		error(string.format("unknown dissector '%s'", dissector.name))
	end
end


haka.context = Context:new()

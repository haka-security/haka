
local Context = class()

function Context.method:__init()
	self.connections = haka.events.EventConnections:new()
end

function Context.method:emitup(data)
end

function Context.method:emitdown(data)
end

Context.method.emit = Context.method.emitdown

function Context.method:signal(emitter, event, ...)
	if not self.connections:signal(emitter, event, ...) then
		return false
	end

	if self.context then
		for _, connections in ipairs(self.context.connections) do
			if not connections:signal(emitter, event, ...) then
				return false
			end
		end
	end

	return true
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

function Context.method:with(context, func)
	local old = self.context
	self.context = context
	func()
	self.context = old
end


local FlowContext = class()

function FlowContext.method:__init()
	self.dissectors = {}
	self.connections = {}
	self.namespace = {}
end

function FlowContext.method:namespace(ref)
	local ret = self.namespace[ref]
	if not ret then
		ret = {}
		self.namespace[ref] = ret
	end
	return ret
end


haka.context = Context:new()

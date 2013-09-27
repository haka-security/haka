
local dissector = {}

dissector.Dissector = class()

function dissector.Dissector:__init()
	self.dissector = self.__class.name
	self.next_dissector = nil
end

function dissector.new(name, hooks)
	local d = class(dissector.Dissector)
	d.name = name
	d.hooks = hooks

	haka.register_dissector({
		name=d.name,
		hooks=d.hooks,
		dissect=function (data)
			local subdata = d.getdata(d, data)
			subdata:dissect(data)
			return subdata
		end
	})

	return d
end

haka.dissector = dissector

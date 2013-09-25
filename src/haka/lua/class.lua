
local BaseClass = {}
BaseClass.__index = function (self, key)
	local v = self.__static[key]
	if v then return v end

	local super = self.__super
	while super do
		v = super.__static[key]
		if v then return v end

		super = super.__super
	end
end


local function new_instance(cls, ...)
	local instance = {}
	instance.__class = cls
	setmetatable(instance, cls)

	local init = cls.__init
	if init then
		init(instance, ...)
	end

	return instance
end

local function clone_instance(instance)
	local c = {}
	for k, v in pairs(instance) do
		c[k] = v
	end
	setmetatable(c, getmetatable(instance))
	return c
end

local BaseObject = {}
BaseObject.__index = BaseObject
BaseObject.__super = nil
BaseObject.__static = {
	new = new_instance,
	clone = clone_instance
}


function class(super)
	super = super or BaseObject

	local cls = {}
	cls.__super = super
	cls.__index = function (self, key)
		local v = self.__class[key]
		if v then return v end
	
		local super = self.__class.__super
		while super do
			v = super[key]
			if v then return v end
	
			super = super.__super
		end
	end
	cls.__static = {}

	setmetatable(cls, BaseClass)
	return cls
end

function static(cls)
	return cls.__static
end

function isa(instance, cls)
	local c = instance.__class
	while c do
		if c == cls then return true end
		c = cls.__super
	end
	return false
end

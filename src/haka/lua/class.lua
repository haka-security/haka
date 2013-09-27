
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

local function iter_class_hierarchy(topcls, cls)
	if cls then return cls.__super
	else return topcls end
end

local function class_hierarchy(cls)
	return iter_class_hierarchy, cls, nil
end

local BaseObject = {
	__super = nil,
	__static = {
		new = new_instance,
		clone = clone_instance
	},
	__property = {
		get = {},
		set = {}
	}
}
BaseObject.__index = BaseObject


function class(super)
	super = super or BaseObject

	local cls = {}
	cls.__super = super
	cls.__index = function (self, key)
		local v
		for c in class_hierarchy(self.__class) do
			v = rawget(c, key)
			if v then return v end
			
			v = c.__property.get[key]
			if v then return v(self) end
		end
	end
	cls.__newindex = function (self, key, value)
		local v
		for c in class_hierarchy(self.__class) do
			v = c.__property.set[key]
			if v then return v(self, value) end
		end
		
		rawset(self, key, value)
	end
	cls.__static = {}
	cls.__property = {
		get = {},
		set = {}
	}

	setmetatable(cls, BaseClass)
	return cls
end

function static(cls)
	return cls.__static
end

function property(cls, type)
	return cls.__property[type]
end

function isa(instance, cls)
	local c = instance.__class
	while c do
		if c == cls then return true end
		c = cls.__super
	end
	return false
end

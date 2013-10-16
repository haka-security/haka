
local function iter_class_hierarchy(topcls, cls)
	if cls then return rawget(cls, 'super')
	else return topcls end
end

local function class_hierarchy(cls)
	return iter_class_hierarchy, cls, nil
end


local BaseClassView = {}
function BaseClassView.__index(self, key)
	local v
	for c in class_hierarchy(rawget(self, '__class')) do
		v = rawget(c, 'method')[key]
		if v then return v end
	end
end

function BaseClassView.__tostring(self)
	return string.format("<class view: %s>", self.cls.__name)
end

local BaseClass = {}
function BaseClass.__index(self, key)
	local v
	for c in class_hierarchy(self) do
		local v = rawget(c, key)
		if v then return v end
	end
end

function BaseClass.__tostring(self)
	return string.format("<class: %s>", self.__name)
end

function BaseClass.view(cls)
	local view = { __class = cls }
	setmetatable(view, BaseClassView)
	return view
end


local function new_instance(cls, ...)
	local instance = {}
	setmetatable(instance, cls)

	local init = cls.method.__init
	if init then
		init(instance, ...)
	end

	return instance
end

local BaseObject = {
	super = nil,
	name = 'BaseObject',
	new = new_instance,
	__class_init = function (self, cls) end,
	method = {
		__init = function (self) end,
		clone = function (self)
			local c = {}
			for k, v in pairs(self) do
				c[k] = v
			end
			setmetatable(c, getmetatable(self))
			return c
		end
	},
	property = {
		get = {},
		set = {}
	}
}
BaseObject.__index = BaseObject
BaseObject.__view = BaseClass.view(BaseObject)

setmetatable(BaseObject, BaseClass)

function classof(instance)
	return getmetatable(instance)
end

function super(instance)
	return rawget(classof(instance).super, '__view')
end


function class(name, super)
	super = super or BaseObject

	local cls = {}
	cls.name = name
	cls.super = super
	cls.__view = BaseClass.view(cls)
	cls.__index = function (self, key)
		local v
		for c in class_hierarchy(classof(self)) do
			local method = rawget(c, 'method')

			v = method[key]
			if v then return v end

			v = rawget(c, 'property').get[key]
			if v then return v(self) end

			v = method['__index']
			if v then
				local v = v(self, key)
				if v then return v end
			end
		end
	end
	cls.__newindex = function (self, key, value)
		local v
		for c in class_hierarchy(classof(self)) do
			v = rawget(c, 'property').set[key]
			if v then return v(self, value) end
		end

		rawset(self, key, value)
	end
	cls.method = {}
	cls.property = {
		get = {},
		set = {}
	}

	setmetatable(cls, BaseClass)

	if super.__class_init then
		super:__class_init(cls)
	end

	return cls
end


function isa(instance, cls)
	local c = classof(instance)
	while c do
		if c == cls then return true end
		c = c.super
	end
	return false
end

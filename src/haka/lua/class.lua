-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
	return string.format("<class view: %s>", rawget(self, '__class').name)
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
	return string.format("<class: %s>", self.name)
end

function BaseClass.view(cls)
	local view = { __class = cls }
	setmetatable(view, BaseClassView)
	return view
end


function new_instance(cls, ...)
	local instance = {}
	setmetatable(instance, cls)

	instance:__init(...)
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
		end,
		addproperty = function (self, name, get, set)
			local property = rawget(self, '__property')
			if not property then
				property = {}
				rawset(self, '__property', property)
			end

			property[name] = { get = get, set = set }
		end
	},
	property = {}
}
BaseObject.__index = BaseObject
BaseObject.__view = BaseClass.view(BaseObject)
setmetatable(BaseObject, BaseClass)

function classof(instance)
	return getmetatable(instance)
end

function super(cls)
	return rawget(cls.super, '__view')
end


function class(name, super)
	super = super or BaseObject

	local cls = {}
	cls.name = name
	cls.super = super
	cls.__view = BaseClass.view(cls)
	cls.__tostring = function (self)
		local convert = self.__tostring
		if convert then
			return convert(self)
		else
			return string.format("<class instance: %s>", classof(self).name)
		end
	end

	cls.method = {}
	cls.property = {}
	cls.__index = function (self, key)
		local v

		-- Dynamic properties
		v = rawget(self, '__property')
		if v then
			v = v[key]
			if v and v.get then
				return v.get(self)
			end
		end

		for c in class_hierarchy(classof(self)) do
			local method = rawget(c, 'method')

			v = method[key]
			if v then return v end

			v = rawget(c, 'property')[key]
			if v and v.get then return v.get(self) end

			v = method['__index']
			if v then
				local v = v(self, key)
				if v then return v end
			end
		end
	end
	cls.__newindex = function (self, key, value)
		local v

		-- Dynamic properties
		v = rawget(self, '__property')
		if v then
			v = v[key]
			if v and v.set then
				return v.set(self, value)
			end
		end

		for c in class_hierarchy(classof(self)) do
			v = rawget(c, 'property')[key]
			if v and v.set then return v.set(self, value) end

			local method = rawget(c, 'method')
			v = method['__newindex']
			if v then
				return v(self, key, value)
			end
		end

		rawset(self, key, value)
	end

	setmetatable(cls, BaseClass)

	super:__class_init(cls)
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

function isclass(cls)
	return getmetatable(cls) == BaseClass
end

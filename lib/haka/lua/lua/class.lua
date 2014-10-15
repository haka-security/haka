-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local module = {}

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

local function build_class_index_table(cls)
	local cache = {}

	for c in class_hierarchy(cls) do
		for name, func in pairs(c) do
			if not cache[name] then
				cache[name] = func
			end
		end
	end

	return cache
end

function BaseClass.__index(self, key)
	local cache = rawget(self, '__class_index')
	if not cache then
		cache = build_class_index_table(self)
		rawset(self, '__class_index', cache)
	end
	return cache[key]
end

function BaseClass.__tostring(self)
	return string.format("<class: %s>", self.name)
end

function BaseClass.view(cls)
	local view = { __class = cls }
	setmetatable(view, BaseClassView)
	return view
end


function module.new_instance(cls, ...)
	local instance = {}
	setmetatable(instance, cls)

	instance:__init(...)
	return instance
end

local BaseObject = {
	super = nil,
	name = 'BaseObject',
	new = module.new_instance,
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

function module.classof(instance)
	return getmetatable(instance)
end

function module.super(cls)
	assert(cls)
	return rawget(cls.super, '__view')
end

local function build_index_table(cls)
	local cache = {}
	local index

	for c in class_hierarchy(cls) do
		for name, method in pairs(rawget(c, 'method')) do
			if name == '__index' then
				if not index then
					index = method
				end
			else
				if not cache[name] then
					cache[name] = function () return method end
				end
			end
		end

		for name, prop in pairs(rawget(c, 'property')) do
			if prop.get and not cache[name] then
				local getter = prop.get
				cache[name] = function (self) return getter(self) end
			end
		end
	end

	return function (self, key)
		local v

		-- Dynamic properties
		v = rawget(self, '__property')
		if v then
			v = v[key]
			if v and v.get then
				return v.get(self)
			end
		end

		v = cache[key]
		if v then return v(self) end

		if index then
			return index(self, key)
		end
	end
end

local function build_newindex_table(cls)
	local cache = {}
	local newindex

	for c in class_hierarchy(cls) do
		if not newindex then
			newindex = rawget(c, 'method').__newindex
		end

		for name, prop in pairs(rawget(c, 'property')) do
			if prop.set and not cache[name] then
				local setter = prop.set
				cache[name] = function (self, value) return setter(self, value) end
			end
		end
	end

	return function (self, key, value)
		local v

		-- Dynamic properties
		v = rawget(self, '__property')
		if v then
			v = v[key]
			if v and v.set then
				return v.set(self, value)
			end
		end

		v = cache[key]
		if v then return v(self, value) end

		if newindex then
			return newindex(self, key, value)
		end

		rawset(self, key, value)
	end
end

function module.class(name, super)
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
			return string.format("<class instance %s: 0x%x>", module.classof(self).name, topointer(self))
		end
	end

	cls.method = {}
	cls.property = {}
	cls.__index = function (self, key)
		cls.__index = build_index_table(cls)
		return cls.__index(self, key)
	end
	cls.__newindex = function (self, key, value)
		cls.__newindex = build_newindex_table(cls)
		return cls.__newindex(self, key, value)
	end

	setmetatable(cls, BaseClass)

	super:__class_init(cls)
	return cls
end

function module.merge(dst, src)
	local psrc = rawget(src, '__property')
	if psrc then
		local pdst = rawget(dst, '__property')
		if not pdst then
			rawset(dst, '__property', rawget(src, '__property'))
		else
			table.merge(pdst, psrc)
		end
		rawset(src, '__property', nil)
	end

	table.merge(dst, src)
end


function module.isa(instance, cls)
	local c = module.classof(instance)
	return module.isaclass(c, cls)
end

function module.isaclass(c, cls)
	while c do
		if c == cls then return true end
		c = c.super
	end
	return false
end

function module.isclass(cls)
	return getmetatable(cls) == BaseClass
end

return module

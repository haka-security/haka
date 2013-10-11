
local grammar = {}


grammar.Entity = class('Entity')

function grammar.Entity.method:as(name)
	local clone = self:clone()
	clone.named = name
	return clone
end

function grammar.Entity.getoption(cls, opt)
	local v

	if cls._options then
		v = cls._options[opt]
		if v then return v end
	end

	local super = cls.__super
	while super do
		if super._options then
			v = super._options[opt]
			if v then return v end
		end

		super = super.__super
	end
end

function grammar.Entity.method:options(options)
	local clone = self:clone()
	for k, v in pairs(options) do
		if type(k) == 'string' then
			local opt = grammar.Entity.getoption(self.__class, k)
			assert(opt, string.format("invalid option '%s'", k))
			opt(clone, v)
		elseif type(v) == 'string' then
			local opt = grammar.Entity.getoption(self.__class, v)
			assert(opt, string.format("invalid option '%s'", v))
			opt(clone)
		else
			error("invalid option")
		end
	end
	return clone
end


grammar.Record = class('Record', grammar.Entity)

function grammar.Record.method:__init(entities)
	self.entities = entities
	self.extra_entities = {}
	self.on_finish = {}
end

function grammar.Record.method:extra(functions)
	for name, func in pairs(functions) do
		if type(name) == 'string' then self.extra_entities[name] = func
		else table.insert(self.on_finish, func) end
	end
	return self
end


grammar.Branch = class('Branch', grammar.Entity)

function grammar.Branch.method:__init(cases, select)
	self.selector = select
	self.cases = cases
end


grammar.Array = class('Array', grammar.Entity)

function grammar.Array.method:__init(entity)
	self.entity = entity
end

grammar.Array._options = {}

function grammar.Array._options.count(self, size)
	self.more = function (self, array)
		return #array < size
	end
end

function grammar.Array._options.untilcond(self, condition)
	self.more = function (self, array)
		if #array > 0 then return false
		else return condition(array[#array]) end
	end
end


grammar.Regex = class('Regex', grammar.Entity)

function grammar.Regex.method:__init(regex)
	self.regex = regex
end

grammar.Regex._options = {}
function grammar.Regex._options.maxsize(self, size) self.maxsize = size end


grammar.Integer = class('Integer', grammar.Entity)

function grammar.Integer.method:__init(bits)
	self.bits = bits
end

grammar.Integer._options = {}
function grammar.Regex._options.endianness(self, endian) self.endian = endian end


grammar.Bytes = class('Bytes', grammar.Entity)

grammar.Bytes._options = {}
function grammar.Bytes._options.chunked(self) self.chunked = true end
function grammar.Bytes._options.count(self, count) self.count = count end


function grammar.record(entities)
	return grammar.Record:new(entities)
end

function grammar.branch(cases, select)
	return grammar.Branch:new(cases, select)
end

function grammar.array(entity)
	return grammar.Array:new(entity)
end

function grammar.re(regex)
	return grammar.Regex:new(regex)
end

function grammar.integer(bits)
	return grammar.Integer:new(bits)
end

function grammar.bytes()
	return grammar.Bytes:new()
end


function grammar.new()
	return {}
end


haka.grammar = grammar

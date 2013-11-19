
--
-- Grammar graph
--

local grammar_dg = {}

grammar_dg.Context = class('DGContext')

grammar_dg.Entity = class('DGEntity')

function grammar_dg.Entity.method:__init()
end

function grammar_dg.Entity.method:next(ctxs, input, bitoffset)
	return self._next, self:apply(ctxs, input, bitoffset) or bitoffset
end

function grammar_dg.Entity.method:add(next)
	self._next = next
end

function grammar_dg.Entity.method:convert(converter, memoize)
	self.converter = converter
	self.memoize = memoize or false
end

function grammar_dg.Entity.method:genproperty(obj, name, get, set)
	if self.converter then
		if self.memoize then
			local memname = '_' .. name .. 'memoize'
			obj:addproperty(name,
				function (this)
					local ret = rawget(this, memname)
					if not ret then
						ret = self.converter.get(get(this))
						rawset(this, memname, ret)
					end
					return ret
				end,
				function (this, newval)
					rawset(this, memname, nil)
					return set(this, self.converter.set(newval))
				end
			)
		else
			obj:addproperty(name,
				function (this)
					return self.converter.get(get(this))
				end,
				function (this, newval)
					return set(this, self.converter.set(newval))
				end
			)
		end
	else
		obj:addproperty(name, get, set)
	end
end

function grammar_dg.Entity.method:lasts(lasts)
	if not self._next then
		table.insert(lasts, self)
		return lasts
	else
		return self._next:lasts(lasts)
	end
end

function grammar_dg.Entity.method:parseall(input, ctx)
	local ctxs = { ctx or grammar_dg.Context:new() }
	local iter = self
	local bitoffset = 0
	while iter do
		iter, bitoffset = iter:next(ctxs, input, bitoffset)
	end
	return ctxs[1]
end

grammar_dg.Control = class('DGControl', grammar_dg.Entity)

grammar_dg.RecordStart = class('DGRecordStart', grammar_dg.Entity)

function grammar_dg.RecordStart.method:__init(name)
	super(grammar_dg.RecordStart).__init(self)
	self.name = name
end

function grammar_dg.RecordStart.method:apply(ctxs)
	if self.name then
		local new = grammar_dg.Context:new()
		
		if self.converter then
			ctxs[#ctxs]:addproperty(self.name,
				function (ctx) return self.converter.get(new) end,
				function (ctx, newvalue) new = self.converter.set(newvalue) end
			)
		else
			ctxs[#ctxs]:addproperty(self.name,
				function (ctx) return new end
			)
		end
	
		table.insert(ctxs, new)
	end
end

grammar_dg.RecordFinish = class('DGRecordFinish', grammar_dg.Control)

function grammar_dg.RecordFinish.method:__init(pop)
	super(grammar_dg.RecordFinish).__init(self)
	self._onfinish = {}
	self._extra = {}
	self._pop = pop
end

function grammar_dg.RecordFinish.method:onfinish(f)
	table.insert(self._onfinish, f)
end

function grammar_dg.RecordFinish.method:extra(name, f)
	self._extra[name] = f
end

function grammar_dg.RecordFinish.method:apply(ctxs)
	local ctx = ctxs[#ctxs]
	if self._pop then
		ctxs[#ctxs] = nil
	end

	for _, func in ipairs(self._onfinish) do
		func(ctxs[1], ctx)
	end

	for name, func in pairs(self._extra) do
		ctx[name] = func(ctxs[1], ctx)
	end
end

grammar_dg.Error = class('DGError', grammar_dg.Control)

function grammar_dg.Error.method:__init(msg)
	super(grammar_dg.Error).__init(self)
	self.msg = msg
end

function grammar_dg.Error.method:apply()
	error(self.msg)
end

grammar_dg.Branch = class('DGBranch', grammar_dg.Control)

function grammar_dg.Branch.method:__init(selector)
	super(grammar_dg.Branch).__init(self)
	self.selector = selector
	self.cases = {}
end

function grammar_dg.Branch.method:case(key, entity)
	self.cases[key] = entity
end

function grammar_dg.Branch.method:next(ctxs, input, bitoffset)
	local next = self.cases[self.selector(ctxs[1])]
	if next then
		return next, bitoffset
	else
		return self._next, bitoffset
	end
end

function grammar_dg.Branch.method:lasts(lasts)
	super(grammar_dg.Branch).lasts(self, lasts)

	for _, entity in pairs(self.cases) do
		entity:lasts(lasts)
	end

	return lasts
end


grammar_dg.Primitive = class('DGPrimitive', grammar_dg.Entity)

function grammar_dg.Primitive.method:apply(ctxs, input, bitoffset)
	return self:parse(ctxs[#ctxs], input, bitoffset)
end

grammar_dg.Number = class('DGNumber', grammar_dg.Primitive)

function grammar_dg.Number.method:__init(size, endian, name)
	super(grammar_dg.Number).__init(self)
	self.size = size
	self.endian = endian
	self.name = name
end

function grammar_dg.Number.method:parse(ctx, input, bitoffset)
	local size, bit = math.ceil((bitoffset + self.size) / 8), (bitoffset + self.size) % 8
	local sub = input:sub(size, false)
	if bit ~= 0 then
		input:advance(size-1)
	else
		input:advance(size)
	end

	if bitoffset == 0 and bit == 0 then
		if self.name then
			self:genproperty(ctx, self.name,
				function (this) return sub:asnumber(self.endian) end,
				function (this, newvalue) return sub:setnumber(newvalue, self.endian) end
			)
		end
	else
		if self.name then
			self:genproperty(ctx, self.name,
				function (this)
					return sub:asbits(bitoffset, self.size, self.endian)
				end,
				function (this, newvalue)
					return sub:setbits(newvalue, bitoffset, self.size, self.endian)
				end
			)
		end
	end

	return bit
end


grammar_dg.Bytes = class('DGBytes', grammar_dg.Primitive)

function grammar_dg.Bytes.method:__init(size, name)
	super(grammar_dg.Bytes).__init(self)
	self.size = size
	self.name = name
end

function grammar_dg.Bytes.method:parse(ctx, input, bitoffset)
	if bitoffset ~= 0 then
		error("byte primitive requires aligned bits")
	end

	local sub = input:sub(self.size or -1)

	if self.name then
		if self.converter then
			ctx:addproperty(self.name,
				function (ctx) return self.converter.get(sub) end,
				function (ctx, newvalue) sub = self.converter.set(newvalue) end
			)
		else
			ctx[self.name] = sub
		end
	end

	return 0
end


local grammar = {}


--
-- Grammar converter
--

grammar.converter = {}

function grammar.converter.mult(val)
	return {
		get = function (x) return x * val end,
		set = function (x) return x / val end
	}
end

grammar.converter.bool = {
	get = function (x) return x ~= 0 end,
	set = function (x) if x then return 1 else return 0 end end
}


--
-- Grammar description
--

grammar.Entity = class('Entity')

function grammar.Entity.method:_as(name)
	local clone = self:clone()
	clone.named = name
	return clone
end

function grammar.Entity.method:convert(converter, memoize)
	local clone = self:clone()
	clone.converter = converter
	clone.memoize = memoize or clone.memoize
	return clone
end

function grammar.Entity.getoption(cls, opt)
	local v

	if cls._options then
		v = cls._options[opt]
		if v then return v end
	end

	local super = cls.super
	while super do
		if super._options then
			v = super._options[opt]
			if v then return v end
		end

		super = super.super
	end
end

function grammar.Entity.method:options(options)
	local clone = self:clone()
	for k, v in pairs(options) do
		if type(k) == 'string' then
			local opt = grammar.Entity.getoption(classof(self), k)
			assert(opt, string.format("invalid option '%s'", k))
			opt(clone, v)
		elseif type(v) == 'string' then
			local opt = grammar.Entity.getoption(classof(self), v)
			assert(opt, string.format("invalid option '%s'", v))
			opt(clone)
		else
			error("invalid option")
		end
	end
	return clone
end

grammar.Entity._options = {}

function grammar.Entity._options.memoize(self)
	self.memoize = true
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

function grammar.Record.method:compile()
	local iter, ret
	
	ret = grammar_dg.RecordStart:new(self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	iter = ret

	for _, entity in ipairs(self.entities) do
		local next = entity:compile()

		if iter then
			for _, last in ipairs(iter:lasts({})) do
				last:add(next)
			end
			iter = next
		else
			ret = next
			iter = ret
		end
	end

	local pop = grammar_dg.RecordFinish:new(self.named)
	for _, f in ipairs(self.on_finish) do
		pop:onfinish(f)
	end
	for name, f in pairs(self.extra_entities) do
		pop:extra(name, f)
	end
	for _, last in ipairs(iter:lasts({})) do
		last:add(pop)
	end

	return ret
end


grammar.Branch = class('Branch', grammar.Entity)

function grammar.Branch.method:__init(cases, select)
	self.selector = select
	self.cases = { default = 'error' }
	for key, value in pairs(cases) do
		self.cases[key] = value
	end
end

function grammar.Branch.method:compile()
	local ret = grammar_dg.Branch:new(self.selector)
	for key, value in pairs(self.cases) do
		if key ~= 'default' then
			ret:case(key, value:compile())
		end
	end

	local default = self.default
	if default == 'error' then
		ret:add(grammar_dg.Error:new("invalid case"))
	elseif default then
		ret:add(default:compile())
	end

	return ret
end


grammar.Array = class('Array', grammar.Entity)

function grammar.Array.method:__init(entity)
	self.entity = entity
end

function grammar.Array.method:compile()
	local inner = self.entity:compile()
	local loop = grammar_dg.Branch(self.more)
	loop.add(true, inner)

	for _, last in ipairs(inner:lasts({})) do
		last:add(loop)
	end

	return inner
end

grammar.Array._options = {}

function grammar.Array._options.count(self, size)
	self.more = function (array)
		return #array < size
	end
end

function grammar.Array._options.untilcond(self, condition)
	self.more = function (array)
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


grammar.Number = class('Number', grammar.Entity)

function grammar.Number.method:__init(bits)
	self.bits = bits
end

function grammar.Number.method:compile()
	local ret = grammar_dg.Number:new(self.bits, self.endian, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	return ret
end

grammar.Number._options = {}
function grammar.Number._options.endianness(self, endian) self.endian = endian end


grammar.Bytes = class('Bytes', grammar.Entity)

function grammar.Bytes.method:compile()
	local ret = grammar_dg.Bytes:new(self.count, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	return ret
end

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

function grammar.number(bits)
	return grammar.Number:new(bits)
end

grammar.flag = grammar.Number:new(1):convert(grammar.converter.bool, false)

function grammar.bytes()
	return grammar.Bytes:new()
end

function grammar.field(name, field)
	return field:_as(name)
end


function grammar.new()
	return {}
end


haka.grammar = grammar

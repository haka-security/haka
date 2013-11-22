
local grammar_dg = {}

--
-- Grammar graph context
--

grammar_dg.Context = class('DGContext')

grammar_dg.ParseContext = class('DGParseContext')

grammar_dg.ParseContext.property.top = {
	get = function (self) return self._ctxs[1] end
}

grammar_dg.ParseContext.property.current = {
	get = function (self) return self._ctxs[#self._ctxs] end
}

grammar_dg.ParseContext.property.offset = {
	get = function (self) return self._offset end
}

function grammar_dg.ParseContext.method:__init(input, topctx)
	self._input = input
	self._offset = 0
	self._bitoffset = 0
	self._ctxs = {}
	self:push(topctx)
end

function grammar_dg.ParseContext.method:parse(entity)
	local iter = entity
	while iter do
		iter = iter:next(self)
	end
	return self._ctxs[1]
end

function grammar_dg.ParseContext.method:pop()
	self._ctxs[#self._ctxs] = nil
end

function grammar_dg.ParseContext.method:push(ctx)
	local new = ctx or grammar_dg.Context:new()
	self._ctxs[#self._ctxs+1] = new
	return new
end


--
-- Grammar graph
--

grammar_dg.Entity = class('DGEntity')

function grammar_dg.Entity.method:__init()
end

function grammar_dg.Entity.method:next(ctx)
	self:apply(ctx)
	return self._next
end

function grammar_dg.Entity.method:_lasts(lasts, set)
	if set[self] then
		return
	end

	set[self] = true

	if not self._next then
		table.insert(lasts, self)
	else
		return self._next:_lasts(lasts, set)
	end
end

function grammar_dg.Entity.method:add(next)
	if not self._next then
		self._next = next
	else
		local lasts = {}
		self:_lasts(lasts, {})
		for _, last in ipairs(lasts) do
			last:add(next)
		end
	end
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

function grammar_dg.Entity.method:parseall(input, ctx)
	local ctx = grammar_dg.ParseContext:new(input, ctx)
	return ctx:parse(self)
end

grammar_dg.Control = class('DGControl', grammar_dg.Entity)

grammar_dg.ContextPop = class('DGContextPop', grammar_dg.Control)

function grammar_dg.ContextPop.method:__init()
	super(grammar_dg.ContextPop).__init(self)
end

function grammar_dg.ContextPop.method:apply(ctx)
	ctx:pop()
end

grammar_dg.RecordStart = class('DGRecordStart', grammar_dg.Control)

function grammar_dg.RecordStart.method:__init(name)
	super(grammar_dg.RecordStart).__init(self)
	self.name = name
end

function grammar_dg.RecordStart.method:apply(ctx)
	if self.name then
		local cur = ctx.current
		local new = ctx:push()
		
		if self.converter then
			cur:addproperty(self.name,
				function (ctx) return self.converter.get(new) end,
				function (ctx, newvalue) new = self.converter.set(newvalue) end
			)
		else
			cur:addproperty(self.name,
				function (ctx) return new end
			)
		end
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

function grammar_dg.RecordFinish.method:apply(ctx)
	local top = ctx.top
	local cur = ctx.current
	if self._pop then
		ctx:pop()
	end

	for _, func in ipairs(self._onfinish) do
		func(cur, ctx)
	end

	for name, func in pairs(self._extra) do
		cur[name] = func(cur, ctx)
	end
end

grammar_dg.ArrayStart = class('DGArrayStart', grammar_dg.Control)

function grammar_dg.ArrayStart.method:__init(name)
	super(grammar_dg.ArrayStart).__init(self)
	self.name = name
end

function grammar_dg.ArrayStart.method:apply(ctx)
	local cur = ctx.current
	local array = ctx:push({})
	if self.name then
		cur[self.name] = array
	end
end

grammar_dg.ArrayPush = class('DGArrayPush', grammar_dg.Control)

function grammar_dg.ArrayPush.method:__init()
	super(grammar_dg.ArrayPush).__init(self)
end

function grammar_dg.ArrayPush.method:apply(ctx)
	local cur = ctx.current
	local new = ctx:push()
	table.insert(cur, new)
end

grammar_dg.Error = class('DGError', grammar_dg.Control)

function grammar_dg.Error.method:__init(msg)
	super(grammar_dg.Error).__init(self)
	self.msg = msg
end

function grammar_dg.Error.method:apply()
	error(self.msg)
end

grammar_dg.Check = class('DGCheck', grammar_dg.Control)

function grammar_dg.Check.method:__init(check, msg)
	super(grammar_dg.Error).__init(self)
	self.check = check
	self.msg = msg
end

function grammar_dg.Check.method:apply(ctx)
	if not self.check(ctx.current, ctx) then
		error(self.msg)
	end
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

function grammar_dg.Branch.method:next(ctx)
	local next = self.cases[self.selector(ctx.current, ctx)]
	if next then return next
	else return self._next end
end

function grammar_dg.Branch.method:_lasts(lasts, set)
	if set[self] then
		return
	end

	for _, entity in pairs(self.cases) do
		entity:_lasts(lasts, set)
	end
	
	return super(grammar_dg.Branch)._lasts(self, lasts, set)
end

grammar_dg.Primitive = class('DGPrimitive', grammar_dg.Entity)

function grammar_dg.Primitive.method:apply(ctx)
	self:parse(ctx.current, ctx._input, ctx)
end

grammar_dg.Number = class('DGNumber', grammar_dg.Primitive)

function grammar_dg.Number.method:__init(size, endian, name)
	super(grammar_dg.Number).__init(self)
	self.size = size
	self.endian = endian
	self.name = name
end

function grammar_dg.Number.method:parse(cur, input, ctx)
	local bitoffset = ctx._bitoffset
	local size, bit = math.ceil((bitoffset + self.size) / 8), (bitoffset + self.size) % 8
	local sub = input:sub(size, false)
	if bit ~= 0 then
		input:advance(size-1)
		ctx._offset = ctx._offset + size-1
	else
		input:advance(size)
		ctx._offset = ctx._offset + size
	end

	ctx._bitoffset = bit

	if bitoffset == 0 and bit == 0 then
		if self.name then
			self:genproperty(cur, self.name,
				function (this) return sub:asnumber(self.endian) end,
				function (this, newvalue) return sub:setnumber(newvalue, self.endian) end
			)
		end
	else
		if self.name then
			self:genproperty(cur, self.name,
				function (this)
					return sub:asbits(bitoffset, self.size, self.endian)
				end,
				function (this, newvalue)
					return sub:setbits(newvalue, bitoffset, self.size, self.endian)
				end
			)
		end
	end
end

grammar_dg.Bits = class('DGBits', grammar_dg.Primitive)

function grammar_dg.Bits.method:__init(size)
	super(grammar_dg.Bits).__init(self)
	self.size = size
end

function grammar_dg.Bits.method:parse(cur, input, ctx)
	local size = self.size(cur, ctx)
	local bitoffset = ctx._bitoffset
	local size, bit = math.ceil((bitoffset + size) / 8), (bitoffset + size) % 8
	if bit ~= 0 then
		input:advance(size-1)
		ctx._offset = ctx._offset + size-1
	else
		input:advance(size)
		ctx._offset = ctx._offset + size
	end

	ctx._bitoffset = bit
end

grammar_dg.Bytes = class('DGBytes', grammar_dg.Primitive)

function grammar_dg.Bytes.method:__init(size, name)
	super(grammar_dg.Bytes).__init(self)
	self.size = size
	self.name = name
end

function grammar_dg.Bytes.method:parse(cur, input, ctx)
	if ctx._bitoffset ~= 0 then
		error("byte primitive requires aligned bits")
	end

	local sub = input:sub(self.size(cur, ctx) or -1)
	ctx._offset = ctx._offset + #sub

	if self.name then
		if self.converter then
			cur:addproperty(self.name,
				function (this) return self.converter.get(sub) end,
				function (this, newvalue) sub = self.converter.set(newvalue) end
			)
		else
			cur[self.name] = sub
		end
	end
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
			iter:add(next)
			iter = next
		else
			ret = next
			iter = ret
		end
	end

	local pop = grammar_dg.RecordFinish:new(self.named ~= nil)
	for _, f in ipairs(self.on_finish) do
		pop:onfinish(f)
	end
	for name, f in pairs(self.extra_entities) do
		pop:extra(name, f)
	end
	iter:add(pop)

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
	local start = grammar_dg.ArrayStart:new(self.named)
	local finish = grammar_dg.ContextPop:new()

	local pop = grammar_dg.ContextPop:new()
	local inner = grammar_dg.ArrayPush:new()
	inner:add(self.entity:compile())
	inner:add(pop)

	local loop = grammar_dg.Branch:new(self.more)
	pop:add(loop)
	loop:case(true, inner)
	loop:add(finish)
	start:add(loop)

	return start
end

grammar.Array._options = {}

function grammar.Array._options.count(self, size)
	self.more = function (array)
		return #array < size
	end
end

function grammar.Array._options.untilcond(self, condition)
	self.more = function (array, ctx)
		if #array == 0 then return not condition(nil, ctx)
		else return not condition(array[#array], ctx) end
	end
end

function grammar.Array._options.whilecond(self, condition)
	self.more = function (array, ctx)
		if #array == 0 then return condition(nil, ctx)
		else return condition(array[#array], ctx) end
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
	if type(self.count) ~= 'function' then
		local count = self.count
		self.count = function (self) return count end
	end

	local ret = grammar_dg.Bytes:new(self.count, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	return ret
end

grammar.Bytes._options = {}
function grammar.Bytes._options.chunked(self) self.chunked = true end
function grammar.Bytes._options.count(self, count) self.count = count end


grammar.Bits = class('Bits', grammar.Entity)

function grammar.Bits.method:__init(bits)
	self.bits = bits
end

function grammar.Bits.method:compile()
	if type(self.bits) ~= 'function' then
		local bits = self.bits
		self.bits = function (self) return bits end
	end

	return grammar_dg.Bits:new(self.bits)
end


grammar.Assert = class('Assert', grammar.Entity)

function grammar.Assert.method:__init(func, msg)
	self.func = func
	self.msg = msg
end

function grammar.Assert.method:compile()
	return grammar_dg.Check:new(self.func, self.msg or "assert failed")
end


function grammar.record(entities)
	return grammar.Record:new(entities)
end

function grammar.branch(cases, select)
	return grammar.Branch:new(cases, select)
end

function grammar.optional(entity, present)
	return grammar.Branch:new({
		[true] = entity,
		default = nil
	}, present)
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

function grammar.padding(args)
	if args.align then
		local align = args.align
		return grammar.Bits:new(function (self, ctx)
			local rem = (ctx.offset * 8 + ctx._bitoffset) % align
			if rem > 0 then return align -rem
			else return 0 end
		end)
	elseif args.size then
		return grammar.Bits:new(args.size)
	else
		error("invalid padding option")
	end
end

function grammar.field(name, field)
	return field:_as(name)
end

function grammar.assert(func, msg)
	return grammar.Assert:new(func, msg)
end


function grammar.new()
	return {}
end


haka.grammar = grammar

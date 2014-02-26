-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local rem = require("regexp/pcre")

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

grammar_dg.ParseContext.property.current_init = {
	get = function (self)
		if self._initctxs then
			return self._initctxs[#self._initctxs]
		end
	end
}

grammar_dg.ParseContext.property.init = {
	get = function (self) return self._initctxs ~= nil end
}

local function revalidate(self)
	local validate = self._validate
	self._validate = {}
	for f, arg in pairs(validate) do
		f(arg)
	end
end

function grammar_dg.ParseContext.method:__init(iter, topctx, init)
	self.iter = iter
	self._bitoffset = 0
	self._marks = {}
	self._ctxs = {}
	self._validate = {}
	self:push(topctx)

	if init then
		self._initctxs = { init }
		self._initctxs_count = 1
	end

	self.current.validate = revalidate
end

function grammar_dg.ParseContext.method:update(iter)
	self.iter:move_to(iter)
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
	if self._initctxs then
		self._initctxs[self._initctxs_count] = nil
		self._initctxs_count = self._initctxs_count-1
	end
end

function grammar_dg.ParseContext.method:push(ctx, name)
	local new = ctx or grammar_dg.Context:new()
	new._validate = self._validate
	self._ctxs[#self._ctxs+1] = new
	if self._initctxs then
		local curinit = self._initctxs[#self._initctxs]
		self._initctxs_count = self._initctxs_count+1
		if curinit then
			self._initctxs[self._initctxs_count] = curinit[name]
		else
			self._initctxs[#self._initctxs+1] = nil
		end
	end
	return new
end

function grammar_dg.ParseContext.method:pushmark()
	self._marks[#self._marks+1] = {
		iter = self.iter:copy(),
		bitoffset = self._bitoffset,
		max_meter = 0,
		max_iter = self.iter:copy(),
		max_bitoffset = self._bitoffset
	}
end

function grammar_dg.ParseContext.method:popmark()
	local mark = self._marks[#self._marks]
	self.iter:move_to(mark.max_iter)
	self._bitoffset = mark.max_bitoffset
	self._marks[#self._marks] = nil
end

function grammar_dg.ParseContext.method:seekmark()
	local mark = self._marks[#self._marks]
	local len = self.iter.meter - mark.iter.meter
	if len >= mark.max_meter then
		mark.max_iter = self.iter:copy()
		mark.max_meter = len
		if self._bitoffset > mark.max_bitoffset then
			mark.max_bitoffset = self._bitoffset
		end
	end
	self.iter:move_to(mark.iter)
	self._bitoffset = mark.bitoffset
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

function grammar_dg.Entity.method:validate(validate)
	self.validate = validate
end

function grammar_dg.Entity.method:genproperty(obj, name, get, set)
	local fget, fset = get, set

	if self.converter then
		if self.memoize then
			local memname = '_' .. name .. '_memoize'
			fget = function (this)
				local ret = rawget(this, memname)
				if not ret then
					ret = self.converter.get(get(this))
					rawset(this, memname, ret)
				end
				return ret
			end
			fset = function (this, newval)
				rawset(this, memname, nil)
				return set(this, self.converter.set(newval))
			end
		else
			fget = function (this)
				return self.converter.get(get(this))
			end
			fset = function (this, newval)
				return set(this, self.converter.set(newval))
			end
		end
	end

	if self.validate then
		local oldget, oldset = fget, fset

		fget = function (this)
			if this._validate[self.validate] then
				self.validate(this)
			end
			return oldget(this)
		end
		fset = function (this, newval)
			if newval == nil then
				this._validate[self.validate] = this
			else
				this._validate[self.validate] = nil
				return oldset(this, newval)
			end
		end
	end

	obj:addproperty(name, fget, fset)
end

function grammar_dg.Entity.method:parseall(input, ctx, init)
	local ctx = grammar_dg.ParseContext:new(input, ctx, init)
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
		local new = ctx:push(nil, self.name)

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

grammar_dg.UnionStart = class('DGUnionStart', grammar_dg.Control)

function grammar_dg.UnionStart.method:__init(name)
	super(grammar_dg.UnionStart).__init(self)
	self.name = name
end

function grammar_dg.UnionStart.method:apply(ctx)
	if self.name then
		local cur = ctx.current
		local new = ctx:push(nil, self.name)
	end

	ctx:pushmark()
end

grammar_dg.UnionRestart = class('DGUnionRestart', grammar_dg.Control)

function grammar_dg.UnionRestart.method:apply(ctx)
	ctx:seekmark()
end

grammar_dg.UnionFinish = class('DGUnionFinish', grammar_dg.Control)

function grammar_dg.UnionFinish.method:__init(pop)
	super(grammar_dg.UnionFinish).__init(self)
	self._pop = pop
end

function grammar_dg.UnionFinish.method:apply(ctx)
	if self._pop then
		ctx:pop()
	end
	ctx:popmark()
end

grammar_dg.ArrayStart = class('DGArrayStart', grammar_dg.Control)

function grammar_dg.ArrayStart.method:__init(name)
	super(grammar_dg.ArrayStart).__init(self)
	self.name = name
end

function grammar_dg.ArrayStart.method:apply(ctx)
	local cur = ctx.current
	local array = ctx:push({}, self.name)
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
	local new = ctx:push(nil, #cur+1)
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

function grammar_dg.Check.method:__init(check)
	super(grammar_dg.Error).__init(self)
	self.check = check
end

function grammar_dg.Check.method:apply(ctx)
	self.check(ctx.current, ctx)
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
	self:parse(ctx.current, ctx.current_init, ctx.iter, ctx)
end

grammar_dg.Number = class('DGNumber', grammar_dg.Primitive)

function grammar_dg.Number.method:__init(size, endian, name)
	super(grammar_dg.Number).__init(self)
	self.size = size
	self.endian = endian
	self.name = name
end

function grammar_dg.Number.method:parse(cur, init, input, ctx)
	local bitoffset = ctx._bitoffset
	local size, bit = math.ceil((bitoffset + self.size) / 8), (bitoffset + self.size) % 8
	local sub = input:copy():sub(size)
	if bit ~= 0 then
		input:advance(size-1)
	else
		input:advance(size)
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
					return sub:setbits(bitoffset, self.size, newvalue, self.endian)
				end
			)
		end
	end

	if self.name and ctx.init then
		if init then
			local initval = init[self.name]
			if initval then
				cur[self.name] = initval
			elseif self.validate then
				cur._validate[self.validate] = cur
			end
		else
			cur[self.name] = nil
		end
	end
end

grammar_dg.Bits = class('DGBits', grammar_dg.Primitive)

function grammar_dg.Bits.method:__init(size)
	super(grammar_dg.Bits).__init(self)
	self.size = size
end

function grammar_dg.Bits.method:parse(cur, init, input, ctx)
	local size = self.size(cur, ctx)
	local bitoffset = ctx._bitoffset + size
	local size, bit = math.ceil(bitoffset / 8), bitoffset % 8
	if bit ~= 0 then
		input:advance(size-1)
	else
		input:advance(size)
	end

	ctx._bitoffset = bit
end

grammar_dg.Bytes = class('DGBytes', grammar_dg.Primitive)

function grammar_dg.Bytes.method:__init(size, name)
	super(grammar_dg.Bytes).__init(self)
	self.size = size
	self.name = name
end

function grammar_dg.Bytes.method:parse(cur, init, input, ctx)
	if ctx._bitoffset ~= 0 then
		error("byte primitive requires aligned bits")
	end

	local sub

	local size = self.size(cur, ctx)
	if size then
		if size < 0 then
			error("byte count must not be negative, got "..size)
		end
		sub = input:sub(size)
	else
		sub = input:sub("all")
	end

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

	if self.name and init then
		local initval = init[self.name]
		if initval then
			sub:replace(initval)
		end
	end
end

grammar_dg.Token = class('DGToken', grammar_dg.Primitive)

function grammar_dg.Token.method:__init(re, name)
	super(grammar_dg.Token).__init(self)
	self.re = re
	self.name = name
end

function grammar_dg.Token.method:parse(cur, init, input, ctx)
	if not ctx.sink then
		ctx.sink = self.re:create_sink()
	end


	-- TODO copy blocking iterator
	local copy = input:copy()
	local match, result = ctx.sink:feed(copy:sub("all"), false)

	if match then
		local string = input:sub(result.size):asstring()
		if self.name then
			cur[self.name] = string
		end
		ctx.sink = nil
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
		set = function (x)
			if x % val ~= 0 then
				error(string.format("invalid value, it must be a multiple of %d", val))
			end
			return x / val
		end
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

function grammar.Entity.method:validate(validate)
	local clone = self:clone()
	clone.validate = validate
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
	if self.validate then ret:validate(self.validate) end
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


grammar.Union = class('Union', grammar.Entity)

function grammar.Union.method:__init(entities)
	self.entities = entities
end

function grammar.Union.method:compile()
	local ret = grammar_dg.UnionStart:new(self.named)

	for _, value in ipairs(self.entities) do
		ret:add(value:compile())
		ret:add(grammar_dg.UnionRestart:new())
	end

	ret:add(grammar_dg.UnionFinish:new(self.named))
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
	if self.validate then ret:validate(self.validate) end
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
	if self.validate then ret:validate(self.validate) end
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

grammar.Token = class('Token', grammar.Entity)

function grammar.Token.method:__init(pattern)
	self.pattern = "^(?:"..pattern..")"
end

function grammar.Token.method:compile()
	local re = rem.re:compile(self.pattern)
	return grammar_dg.Token:new(re, self.named)
end

grammar.Verify = class('Verify', grammar.Entity)

function grammar.Verify.method:__init(func)
	self.func = func
end

function grammar.Verify.method:compile()
	return grammar_dg.Check:new(self.func)
end


function grammar.record(entities)
	return grammar.Record:new(entities)
end

function grammar.union(entities)
	return grammar.Union:new(entities)
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

function grammar.token(pattern)
	return grammar.Token:new(pattern)
end

grammar.flag = grammar.number(1):convert(grammar.converter.bool, false)

function grammar.bytes()
	return grammar.Bytes:new()
end

function grammar.padding(args)
	if args.align then
		local align = args.align
		return grammar.Bits:new(function (self, ctx)
			local rem = (ctx.iter.meter * 8 + ctx._bitoffset) % align
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

function grammar.verify(func, msg)
	if msg then
		return grammar.Verify:new(function (self, ctx)
			if not func(self, ctx) then
				error(msg)
			end
		end)
	else
		return grammar.Verify:new(func)
	end
end


function grammar.new()
	return {}
end


haka.grammar = grammar

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local rem = require("regexp/pcre")

local grammar_dg = {}
local grammar = {}

--
-- Parsing Error
--

local ParseError = class("ParseError")

function ParseError.method:__init(iterator, rule, description, ...)
	self.iterator = iterator
	self.rule = rule
	self.description = description
end

function ParseError.method:__tostring()
	local string = {"parse error"}

	table.insert(string, " at byte ")
	table.insert(string, self.iterator.meter)

	if self.rule then
		table.insert(string, " in ")
		table.insert(string, self.rule)
	end

	table.insert(string, ": ")
	table.insert(string, self.description)

	return table.concat(string)
end


--
-- Grammar result
--

grammar.Result = class('Result')

grammar.ArrayResult = class('ArrayResult', grammar.Result)

function grammar.ArrayResult.method:_init(iter, entity, create)
	rawset(self, '_begin', iter)
	rawset(self, '_entity', entity)
	rawset(self, '_create', create)
end

function grammar.ArrayResult.method:remove(el)
	local index
	if type(el) == 'number' then
		index = el
	else
		for i, value in ipairs(self) do
			if value == el then
				index = i
				break
			end
		end
	end

	if not index then
		return
	end

	self[index]._sub:erase()
	table.remove(self, index)
end

function grammar.ArrayResult.method:append(init)
	local result = grammar.Result:new()
	if not self._create then
		error("missing create function in array options")
	end
	local vbuf = self._create(result, self._entity, init)
	if #self > 0 then
		local sub = rawget(self, #self)._sub
		rawset(result, '_sub', sub:pos('end'):insert(vbuf))
		table.insert(self, result)
	else
		rawget(self, '_begin'):insert(vbuf)
	end
end

--
-- Parse Context
--

grammar_dg.ParseContext = class('DGParseContext')

grammar_dg.ParseContext.property.top = {
	get = function (self) return self._results[1] end
}

grammar_dg.ParseContext.property.result = {
	get = function (self) return self._results[#self._results] end
}

grammar_dg.ParseContext.property.prev_result = {
	get = function (self) return self._results[#self._results-1] end
}

grammar_dg.ParseContext.property.current_init = {
	get = function (self)
		if self._initresults then
			return self._initresults[#self._initresults]
		end
	end
}

grammar_dg.ParseContext.property.init = {
	get = function (self) return self._initresults ~= nil end
}

grammar_dg.ParseContext.property.retain_mark = {
	get = function (self)
		return self._retain_mark[#self._retain_mark]
	end
}

local function revalidate(self)
	local validate = self._validate
	self._validate = {}
	for f, arg in pairs(validate) do
		f(arg)
	end
end

function grammar_dg.ParseContext.method:__init(iter, topresult, init)
	self.iter = iter
	self._bitoffset = 0
	self._marks = {}
	self._results = {}
	self._validate = {}
	self._retain_mark = {}
	self:push(topresult)

	if init then
		self._initresults = { init }
		self._initresults_count = 1
	end

	self.result.validate = revalidate
end

function grammar_dg.ParseContext.method:update(iter)
	self.iter:move_to(iter)
end

function grammar_dg.ParseContext.method:lookahead()
	local iter = self.iter:copy()
	local sub = self.iter:sub(1)
	if sub then
		local la = sub:asnumber()
		self:update(iter)
		return la
	else
		return -1
	end
end

function grammar_dg.ParseContext.method:parse(entity)
	local iter = entity
	local err
	while iter do
		err = iter:_apply(self)
		if err then
			break
		end
		iter = iter:next(self)
	end
	return self._results[1], err
end

function grammar_dg.ParseContext.method:init(entity)
	local iter = entity
	local err
	while iter do
		err = iter:init(self)
		if err then
			break
		end
		iter = iter:next(self)
	end
	return self._results[1], err
end

function grammar_dg.ParseContext.method:pop()
	self._results[#self._results] = nil

	if self._initresults then
		self._initresults[self._initresults_count] = nil
		self._initresults_count = self._initresults_count-1
	end
end

function grammar_dg.ParseContext.method:push(result)
	local new = result or grammar.Result:new()
	rawset(new, '_validate', self._validate)
	self._results[#self._results+1] = new
	if self._initresults then
		local curinit = self._initresults[#self._initresults]
		self._initresults_count = self._initresults_count+1
		if curinit then
			self._initresults[self._initresults_count] = curinit[name]
		else
			self._initresults[#self._initresults+1] = nil
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

function grammar_dg.ParseContext.method:error(position, field, description, ...)
	local context = {}
	description = string.format(description, ...)

	position = position or self.iter
	local iter = position:copy()

	haka.log.debug("grammar", "parse error at byte %d for field %s in %s: %s",
		position.meter, field.id or "<unknown>", field.rule or "<unknown>",
		description)
	haka.log.debug("grammar", "parse error context: %s...", safe_string(iter:sub(100):asstring()))

	return ParseError:new(position, field.rule, description)
end


--
-- Grammar graph
--

grammar_dg.Entity = class('DGEntity')

function grammar_dg.Entity.method:__init(rule, id)
	self.rule = rule
	self.id = id
end

function grammar_dg.Entity.method:next(ctx)
	return self._next
end

function grammar_dg.Entity.method:init(ctx)
	return self:_apply(ctx)
end

function grammar_dg.Entity.method:_apply(ctx)
	return nil
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

function grammar_dg.Entity.method:parse(input, result, context)
	local ctx = grammar_dg.ParseContext:new(input, result)
	ctx.user = context
	return ctx:parse(self)
end

function grammar_dg.Entity.method:create(input, ctx, init)
	local ctx = grammar_dg.ParseContext:new(input, ctx, init)
	return ctx:init(self)
end

grammar_dg.Control = class('DGControl', grammar_dg.Entity)

grammar.ResultPop = class('DGResultPop', grammar_dg.Control)

function grammar.ResultPop.method:__init()
	super(grammar.ResultPop).__init(self)
end

function grammar.ResultPop.method:_apply(ctx)
	ctx:pop()
end

grammar_dg.RecordStart = class('DGRecordStart', grammar_dg.Control)

function grammar_dg.RecordStart.method:__init(name)
	super(grammar_dg.RecordStart).__init(self)
	self.name = name
end

function grammar_dg.RecordStart.method:_apply(ctx)
	if self.name then
		local res = ctx.result
		local new = ctx:push(nil, self.name)

		if self.converter then
			res:addproperty(self.name,
				function (ctx) return self.converter.get(new) end,
				function (ctx, newvalue) new = self.converter.set(newvalue) end
			)
		else
			res:addproperty(self.name,
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

function grammar_dg.RecordFinish.method:_apply(ctx)
	local top = ctx.top
	local res = ctx.result
	if self._pop then
		ctx:pop()
	end

	for _, func in ipairs(self._onfinish) do
		func(res, ctx)
	end

	for name, func in pairs(self._extra) do
		res[name] = func(res, ctx)
	end
end

grammar_dg.UnionStart = class('DGUnionStart', grammar_dg.Control)

function grammar_dg.UnionStart.method:__init(name, rule)
	super(grammar_dg.UnionStart).__init(self, rule)
	self.name = name
end

function grammar_dg.UnionStart.method:_apply(ctx)
	if self.name then
		local res = ctx.result
		local new = ctx:push(nil, self.name)
	end

	ctx:pushmark()
end

grammar_dg.UnionRestart = class('DGUnionRestart', grammar_dg.Control)

function grammar_dg.UnionRestart.method:_apply(ctx)
	ctx:seekmark()
end

grammar_dg.UnionFinish = class('DGUnionFinish', grammar_dg.Control)

function grammar_dg.UnionFinish.method:__init(pop)
	super(grammar_dg.UnionFinish).__init(self)
	self._pop = pop
end

function grammar_dg.UnionFinish.method:_apply(ctx)
	if self._pop then
		ctx:pop()
	end
	ctx:popmark()
end

grammar_dg.ArrayStart = class('DGArrayStart', grammar_dg.Control)

function grammar_dg.ArrayStart.method:__init(name, rule, entity, create, resultclass)
	super(grammar_dg.ArrayStart).__init(self, rule)
	self.name = name
	self.entity = entity
	self.create = create
	self.resultclass = resultclass or grammar.ArrayResult
end

function grammar_dg.ArrayStart.method:_apply(ctx)
	local res = ctx.result
	if self.name then
		local result = self.resultclass:new()
		result:_init(ctx.iter:copy(), self.entity, self.create)
		local array = ctx:push(result, self.name)
		res[self.name] = array
	else
		ctx:push(nil, nil)
	end
end

grammar_dg.ArrayFinish = class('DGArrayFinish', grammar_dg.Control)

function grammar_dg.ArrayFinish.method:__init()
	super(grammar_dg.ArrayFinish).__init(self)
end

function grammar_dg.ArrayFinish.method:apply(ctx)
	ctx:pop()
end

grammar_dg.ArrayPush = class('DGArrayPush', grammar_dg.Control)

function grammar_dg.ArrayPush.method:__init()
	super(grammar_dg.ArrayPush).__init(self)
end

function grammar_dg.ArrayPush.method:_apply(ctx)
	local res = ctx.result
	if isa(res, grammar.ArrayResult) then
		rawset(res, '_entitybegin', ctx.iter:copy())
	end
	local new = ctx:push(nil, #res+1)
	table.insert(res, new)
end

grammar_dg.ArrayPop = class('DGArrayPop', grammar_dg.Control)

function grammar_dg.ArrayPop.method:__init()
	super(grammar_dg.ArrayPop).__init(self)
end

function grammar_dg.ArrayPop.method:_apply(ctx)
	local entityresult = ctx.result
	ctx:pop()
	local arrayresult = ctx.result
	if isa(arrayresult, grammar.ArrayResult) then
		rawset(entityresult, '_sub', haka.vbuffer_sub(arrayresult._entitybegin, ctx.iter))
		rawset(arrayresult, '_entitybegin', nil)
		ctx.iter:split()
	end
end

grammar_dg.Error = class('DGError', grammar_dg.Control)

function grammar_dg.Error.method:__init(id, msg)
	super(grammar_dg.Error).__init(self, nil, id)
	self.msg = msg
end

function grammar_dg.Error.method:_apply(ctx)
	return ctx:error(nil, self, self.msg)
end

grammar_dg.Execute = class('DGExecute', grammar_dg.Control)

function grammar_dg.Execute.method:__init(rule, id, callback)
	super(grammar_dg.Error).__init(self, rule, id)
	self.callback = callback
end

function grammar_dg.Execute.method:_apply(ctx)
	self.callback(ctx.result, ctx)
end

grammar_dg.Retain = class('DGRetain', grammar_dg.Control)

function grammar_dg.Retain.method:__init(readonly)
	super(grammar_dg.Error).__init(self)
	self.readonly = readonly
end

function grammar_dg.Retain.method:_apply(ctx)
	-- We try to mark the incoming data, so wait for them
	-- to arrive before marking the end of a previous chunk
	ctx.iter:wait()

	local mark = ctx.iter:copy()
	mark:mark(self.readonly)
	table.insert(ctx._retain_mark, mark)
end

grammar_dg.Release = class('DGRelease', grammar_dg.Control)

function grammar_dg.Release.method:_apply(ctx)
	local mark = ctx._retain_mark[#ctx._retain_mark]
	ctx._retain_mark[#ctx._retain_mark] = nil
	mark:unmark()
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
	local next = self.cases[self.selector(ctx.result, ctx)]
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

function grammar_dg.Primitive.method:_apply(ctx)
	return self:_parse(ctx.result, ctx.iter, ctx)
end

function grammar_dg.Primitive.method:init(ctx)
	return self:_init(ctx.result, ctx.iter, ctx, ctx.current_init)
end

grammar_dg.Number = class('DGNumber', grammar_dg.Primitive)

function grammar_dg.Number.method:__init(rule, id, size, endian, name)
	super(grammar_dg.Number).__init(self, rule, id)
	self.size = size
	self.endian = endian
	self.name = name
end

function grammar_dg.Number.method:_parse(res, input, ctx)
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
			self:genproperty(res, self.name,
				function (this) return sub:asnumber(self.endian) end,
				function (this, newvalue) return sub:setnumber(newvalue, self.endian) end
			)
		end
	else
		if self.name then
			self:genproperty(res, self.name,
				function (this)
					return sub:asbits(bitoffset, self.size, self.endian)
				end,
				function (this, newvalue)
					return sub:setbits(bitoffset, self.size, newvalue, self.endian)
				end
			)
		end
	end
end

function grammar_dg.Number.method:_init(res, input, ctx, init)
	self:_parse(res, input, ctx)

	if self.name and ctx.init then
		if init then
			local initval = init[self.name]
			if initval then
				res[self.name] = initval
			elseif self.validate then
				res._validate[self.validate] = res
			end
		else
			res[self.name] = nil
		end
	end
end

grammar_dg.Bits = class('DGBits', grammar_dg.Primitive)

function grammar_dg.Bits.method:__init(rule, id, size)
	super(grammar_dg.Bits).__init(self, rule, id)
	self.size = size
end

function grammar_dg.Bits.method:_parse(res, input, ctx)
	local size = self.size(res, ctx)
	local bitoffset = ctx._bitoffset + size
	local size, bit = math.ceil(bitoffset / 8), bitoffset % 8
	if bit ~= 0 then
		input:advance(size-1)
	else
		input:advance(size)
	end

	ctx._bitoffset = bit
end

function grammar_dg.Bits.method:_init(res, input, ctx, init)
end

grammar_dg.Bytes = class('DGBytes', grammar_dg.Primitive)

function grammar_dg.Bytes.method:__init(rule, id, size, name, chunked_callback)
	super(grammar_dg.Bytes).__init(self, rule, id)
	self.size = size
	self.name = name
	self.chunked_callback = chunked_callback
end

function grammar_dg.Bytes.method:_parse(res, iter, ctx)
	if ctx._bitoffset ~= 0 then
		return ctx:error(nil, self, "byte primitive requires aligned bits")
	end

	local sub
	local size = self.size(res, ctx)
	if size then
		if size < 0 then
			return ctx:error(nil, self, "byte count must not be negative, got %d", size)
		end
	else
		size = 'all'
	end

	if self.chunked_callback then
		if size == 0 then
			self.chunked_callback(res, nil, true, ctx)
		else
			while (size == 'all' or size > 0) and iter:wait() do
				if size ~= 'all' then
					local subsize = iter:available()
					if subsize > size then
						sub = iter:sub(size, true)
						subsize = size
					else
						sub = iter:sub('available')
					end

					size = size - subsize
				else
					sub = iter:sub('available')
				end

				self.chunked_callback(res, sub, size == 0 or iter.iseof, ctx)
				if not sub then break end
			end
		end
	else
		sub = iter:sub(size)

		if self.name then
			if self.converter then
				res:addproperty(self.name,
					function (this) return self.converter.get(sub) end,
					function (this, newvalue) sub = self.converter.set(newvalue) end
				)
			else
				res[self.name] = sub
			end
		end
	end
end

function grammar_dg.Bytes.method:_init(res, input, ctx, init)
	if self.name and init then
		local initval = init[self.name]
		if initval then
			sub:replace(initval)
		end
	end
end

grammar_dg.Token = class('DGToken', grammar_dg.Primitive)

function grammar_dg.Token.method:__init(rule, id, pattern, re, name)
	super(grammar_dg.Token).__init(self, rule, id)
	self.pattern = pattern
	self.re = re
	self.name = name
end

function grammar_dg.Token.method:_parse(res, iter, ctx)
	if not ctx.sink then
		ctx.sink = self.re:create_sink()
	end

	local begin

	while true do
		iter:wait()

		if not begin then
			begin = iter:copy()
			begin:mark(haka.packet.mode() == haka.packet.PASSTHROUGH)
		end

		local sub = iter:sub('available')

		local match, result = ctx.sink:feed(sub or "", iter.iseof)
		if not match and not ctx.sink:ispartial() then break end

		if match then
			iter:move_to(begin)
			iter:unmark()

			local sub = iter:sub(result.size, true)
			local string = sub:asstring()

			if self.converter then
				string = self.converter.get(string)
			end

			if self.name then
				res:addproperty(self.name,
					function (this)
						return string
					end,
					function (this, newvalue)
						if self.converter then
							newvalue = self.converter.set(newvalue)
						end
						sub:setstring(newvalue)
						string = newvalue
					end
				)
			end
			ctx.sink = nil
			return
		end

		if not sub then break end
	end

	-- No match found return an error
	return ctx:error(begin:copy(), self, "token /%s/ doesn't match", self.pattern)

end

function grammar_dg.Token.method:_init(res, input, ctx, init)
	-- Should have been initialized
	return self:_parse(res, input, ctx)
end


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

function grammar.converter.tonumber(format, base)
	return {
		get = function (x) return tonumber(x, base) end,
		set = function (x) return string.format(format, x) end
	}
end


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

function grammar.Record.method:compile(rule, id)
	local iter, ret

	ret = grammar_dg.RecordStart:new(self.named, self.rule)
	if self.converter then ret:convert(self.converter, self.memoize) end
	if self.validate then ret:validate(self.validate) end
	iter = ret

	for i, entity in ipairs(self.entities) do
		local next = entity:compile(self.rule or rule, i)

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

function grammar.Union.method:compile(rule, id)
	local ret = grammar_dg.UnionStart:new(self.named, self.rule)

	for i, value in ipairs(self.entities) do
		ret:add(value:compile(self.rule or rule, i))
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

function grammar.Branch.method:compile(rule, id)
	local ret = grammar_dg.Branch:new(self.selector)
	for key, value in pairs(self.cases) do
		if key ~= 'default' then
			ret:case(key, value:compile(self.rule or rule, key))
		end
	end

	local default = self.cases.default
	if default == 'error' then
		ret:add(grammar_dg.Error:new(self.rule or rule, "invalid case"))
	elseif default ~= 'continue' then
		ret:add(default:compile(self.rule or rule, 'default'))
	end

	return ret
end


grammar.Array = class('Array', grammar.Entity)

function grammar.Array.method:__init(entity)
	self.entity = entity
end

function grammar.Array.method:compile(rule, id)
	local entity = self.entity:compile(self.rule or rule, id)
	local start = grammar_dg.ArrayStart:new(self.named, self.rule, entity, self.create, self.resultclass)
	local finish = grammar.ResultPop:new()

	local pop = grammar_dg.ArrayPop:new()
	local inner = grammar_dg.ArrayPush:new()
	inner:add(self.entity:compile(self.rule or rule, id))
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
	local sizefunc

	if type(size) ~= 'function' then
		sizefunc = function () return size end
	else
		sizefunc = size
	end

	self.more = function (array, ctx)
		return #array < sizefunc(ctx.prev_result, ctx)
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

function grammar.Array._options.create(self, f)
	self.create = f
end

function grammar.Array._options.result(self, resultclass)
	self.resultclass = resultclass
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

function grammar.Number.method:compile(rule, id)
	local ret = grammar_dg.Number:new(rule, id, self.bits, self.endian, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	if self.validate then ret:validate(self.validate) end
	return ret
end

grammar.Number._options = {}
function grammar.Number._options.endianness(self, endian) self.endian = endian end


grammar.Bytes = class('Bytes', grammar.Entity)

function grammar.Bytes.method:compile(rule, id)
	if type(self.count) ~= 'function' then
		local count = self.count
		self.count = function (self) return count end
	end

	local ret = grammar_dg.Bytes:new(rule, id, self.count, self.named, self.chunked)
	if self.converter then ret:convert(self.converter, self.memoize) end
	if self.validate then ret:validate(self.validate) end
	return ret
end

grammar.Bytes._options = {}
function grammar.Bytes._options.chunked(self, callback) self.chunked = callback end
function grammar.Bytes._options.count(self, count) self.count = count end


grammar.Bits = class('Bits', grammar.Entity)

function grammar.Bits.method:__init(bits)
	self.bits = bits
end

function grammar.Bits.method:compile(rule, id)
	if type(self.bits) ~= 'function' then
		local bits = self.bits
		self.bits = function (self) return bits end
	end

	return grammar_dg.Bits:new(rule, id, self.bits)
end

grammar.Token = class('Token', grammar.Entity)

function grammar.Token.method:__init(pattern)
	self.pattern = pattern
end

function grammar.Token.method:compile(rule, id)
	if not self.re then
		self.re = rem.re:compile("^(?:"..self.pattern..")")
	end
	local ret = grammar_dg.Token:new(rule, id, self.pattern, self.re, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	return ret
end

grammar.Execute = class('Execute', grammar.Entity)

function grammar.Execute.method:__init(func)
	self.func = func
end

function grammar.Execute.method:compile(rule, id)
	return grammar_dg.Execute:new(rule, id, self.func)
end

grammar.Retain = class('Retain', grammar.Entity)

function grammar.Retain.method:__init(readonly)
	self.readonly = readonly
end

function grammar.Retain.method:compile(rule, id)
	return grammar_dg.Retain:new(self.readonly)
end

grammar.Release = class('Release', grammar.Entity)

function grammar.Release.method:compile(rule, id)
	return grammar_dg.Release:new()
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
		default = 'continue'
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
	return grammar.Execute:new(function (self, ctx)
		if not func(self, ctx) then
			error(msg)
		end
	end)
end

function grammar.execute(func)
	return grammar.Execute:new(func)
end

function grammar.retain(readonly)
	if readonly == nil then
		readonly = haka.packet.mode() == haka.packet.PASSTHROUGH
	end

	return grammar.Retain:new(readonly)
end

grammar.release = grammar.Release:new()


--
-- Grammar
--

local grammar_class = class("Grammar")

function grammar_class.method:__init(name)
	self._name = name
	self._rules = {}
end

function grammar_class.method:__index(name)
	return self._rules[name]
end

function grammar_class.method:__newindex(key, value)
	if isa(value, grammar.Entity) then
		value.rule = key
		self._rules[key] = value
	else
		rawset(self, key, value)
	end
end

function grammar.new(name)
	return grammar_class:new(name)
end


haka.grammar = grammar

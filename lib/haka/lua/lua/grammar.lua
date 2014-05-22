-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local rem = require("regexp/pcre")

local grammar_dg = {}
local grammar_int = {}
local grammar = {}

--
-- Parsing Error
--

local ParseError = class.class("ParseError")

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

grammar.Result = class.class('Result')

grammar.ArrayResult = class.class('ArrayResult', grammar.Result)

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

grammar_dg.ParseContext = class.class('DGParseContext')

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

function grammar_dg.ParseContext.method:result(idx)
	idx = idx or -1

	if idx < 0 then
		idx = #self._results + 1 + idx
		if idx < 0 then
			error("invalid result index")
		end
	else
		if idx > #self._results then
			error("invalid result index")
		end
	end

	if idx <= 0 then
		error("invalid result index")
	end

	return self._results[idx]
end

function grammar_dg.ParseContext.method:mark(readonly)
	local mark = self.iter:copy()
	mark:mark(readonly)
	table.insert(self._retain_mark, mark)
end

function grammar_dg.ParseContext.method:unmark()
	local mark = self.retain_mark
	self._retain_mark[#self._retain_mark] = nil
	mark:unmark()
end

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
	self._catches = {}
	self._results = {}
	self._validate = {}
	self._retain_mark = {}
	self:push(topresult)

	if init then
		self._initresults = { init }
		self._initresults_count = 1
	end

	self:result(1).validate = revalidate

	self.iter.meter = 0
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
		iter:_trace(self.iter)
		err = iter:_apply(self)
		if err then
			iter = self:catch()
			if not iter then
				break
			else
				err = nil
			end
		else
			iter = iter:next(self)
		end
	end
	return self._results[1], err
end

function grammar_dg.ParseContext.method:create(entity)
	local iter = entity
	local err
	while iter do
		err = iter:_create(self)
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

function grammar_dg.ParseContext.method:push(result, name)
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

function grammar_dg.ParseContext.method:popmark(seek)
	local seek = seek or false
	local mark = self._marks[#self._marks]

	if seek then
		self.iter:move_to(mark.max_iter)
		self._bitoffset = mark.max_bitoffset
	end

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

function grammar_dg.ParseContext.method:pushcatch(entity)
	self:mark(false)
	self:pushmark()

	self._catches[#self._catches+1] = {
		entity = entity,
		retain_count = #self._retain_mark,
		mark_count = #self._marks,
	}
end

function grammar_dg.ParseContext.method:catch()
	local catch = self._catches[#self._catches]
	if not catch then
		return nil
	end

	-- Unmark each retain started in failing case
	while #self._retain_mark > catch.retain_count do
		self:unmark()
	end

	-- remove all marks done in failing case
	while #self._marks > catch.mark_count do
		self:popmark(false)
	end


	self:seekmark()
	self:popcatch()

	-- Should be done in Try entity but we can't
	self:pop()

	return catch.entity
end

function grammar_dg.ParseContext.method:popcatch(entity)
	local catch = self._catches[#self._catches]

	self:popmark(false)
	self:unmark()

	self._catches[#self._catches] = nil

	return catch
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

grammar_dg.Entity = class.class('DGEntity')

function grammar_dg.Entity.method:__init(rule, id)
	self.rule = rule
	self.id = id
end

function grammar_dg.Entity.method:next(ctx)
	return self._next
end

function grammar_dg.Entity.method:_create(ctx)
	return self:_apply(ctx)
end

function grammar_dg.Entity.method:_apply(ctx)
	return nil
end

function grammar_dg.Entity.method:_nexts(list)
	if self._next then
		table.insert(list, self._next)
	end
end

function grammar_dg.Entity.method:_add(next, set)
	if set[self] then
		return
	end

	set[self] = true

	local nexts = {}
	self:_nexts(nexts)

	for _, entity in ipairs(nexts) do
		entity:_add(next, set)
	end

	if not self._next then
		self._next = next
	end
end

function grammar_dg.Entity.method:add(next)
	self:_add(next, {})
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
	return ctx:create(self)
end

function grammar_dg.Entity.method:_dump_graph_descr()
end

function grammar_dg.Entity.method:_dump_graph_options()
	return ""
end

function grammar_dg.Entity.method:_dump_graph_node(file, ref)
	local label, extlabel

	if self.name then
		label = string.format("%s: %s", class.classof(self).name, self.name)
	else
		label = class.classof(self).name
	end

	extlabel = self:_dump_graph_descr()
	if extlabel then
		label = string.format("%s\\n%s", label, extlabel)
	end

	file:write(string.format('%s [label="%s"%s];\n', ref[self], label, self:_dump_graph_options()))
end

function grammar_dg.Entity.method:_dump_graph_edges(file, ref)
	if self._next then
		self._next:_dump_graph(file, ref)
		file:write(string.format('%s -> %s;\n', ref[self], ref[self._next]))
	end
end

function grammar_dg.Entity.method:_dump_graph(file, ref)
	if not ref[self] then
		ref[self] = string.format("N_%d", ref._index)
		ref._index = ref._index+1
		self:_dump_graph_node(file, ref)
		return self:_dump_graph_edges(file, ref)
	end
end

function grammar_dg.Entity.method:dump_graph(file)
	file:write("digraph grammar {\n")

	local ref = {}
	ref._index = 1
	self:_dump_graph(file, ref)

	file:write("}\n")
end

function grammar_dg.Entity.method:trace(position, msg, ...)
	if self.id then
		haka.log.debug("grammar", "in rule %s field %s: %s\n\tat byte %d: %s...",
			self.rule or "<unknown>", self.name or self.id,
			string.format(msg, ...), position.meter,
			safe_string(position:copy():sub(20):asstring()))
	end
end

function grammar_dg.Entity.method:_trace(position)
	local name = class.classof(self).trace_name
	if name then
		local descr = self:_dump_graph_descr()
		if descr then
			self:trace(position, 'parsing %s %s', name, descr)
		else
			self:trace(position, 'parsing %s', name)
		end
	end
end

grammar_dg.Control = class.class('DGControl', grammar_dg.Entity)

function grammar_dg.Control.method:_dump_graph_options()
	return ',fillcolor="#dddddd",style="filled"'
end

grammar_dg.ResultPop = class.class('DGResultPop', grammar_dg.Control)

function grammar_dg.ResultPop.method:__init()
	class.super(grammar_dg.ResultPop).__init(self)
end

function grammar_dg.ResultPop.method:_apply(ctx)
	ctx:pop()
end

grammar_dg.RecordStart = class.class('DGRecordStart', grammar_dg.Control)

grammar_dg.RecordStart.trace_name = 'record'

function grammar_dg.RecordStart.method:__init(rule, id, name)
	class.super(grammar_dg.RecordStart).__init(self, rule, id)
	self.name = name
end

function grammar_dg.RecordStart.method:_apply(ctx)
	if self.name then
		local res = ctx:result()
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

grammar_dg.RecordFinish = class.class('DGRecordFinish', grammar_dg.Control)

function grammar_dg.RecordFinish.method:__init(pop)
	class.super(grammar_dg.RecordFinish).__init(self)
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
	local top = ctx:result(1)
	local res = ctx:result()
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

grammar_dg.UnionStart = class.class('DGUnionStart', grammar_dg.Control)

grammar_dg.UnionStart.trace_name = 'union'

function grammar_dg.UnionStart.method:__init(rule, id, name)
	class.super(grammar_dg.UnionStart).__init(self, rule, id)
	self.name = name
end

function grammar_dg.UnionStart.method:_apply(ctx)
	if self.name then
		local res = ctx:result()
		local new = ctx:push(nil, self.name)
	end

	ctx:mark(false)
	ctx:pushmark()
end

grammar_dg.UnionRestart = class.class('DGUnionRestart', grammar_dg.Control)

function grammar_dg.UnionRestart.method:_apply(ctx)
	ctx:seekmark()
end

grammar_dg.UnionFinish = class.class('DGUnionFinish', grammar_dg.Control)

function grammar_dg.UnionFinish.method:__init(pop)
	class.super(grammar_dg.UnionFinish).__init(self)
	self._pop = pop
end

function grammar_dg.UnionFinish.method:_apply(ctx)
	if self._pop then
		ctx:pop()
	end
	ctx:popmark()
	ctx:unmark()
end

grammar_dg.Try = class.class('DGTry', grammar_dg.Control)

function grammar_dg.Try.method:__init(rule, id, name)
	class.super(grammar_dg.Try).__init(self, rule, id)
	self.name = name
	self._catch = nil
end

function grammar_dg.Try.method:catch(catch)
	self._catch = catch
end

function grammar_dg.Try.method:_dump_graph_edges(file, ref)
	self._next:_dump_graph(file, ref)
	file:write(string.format('%s -> %s;\n', ref[self], ref[self._next]))
	self._catch:_dump_graph(file, ref)
	file:write(string.format('%s -> %s [label="catch"];\n', ref[self], ref[self._catch]))
end

function grammar_dg.Try.method:_apply(ctx)
	-- Create a result ctx even if self.name is null
	-- in order to be able to delete it if case fail
	ctx:push(nil, self.name)
	ctx:pushcatch(self._catch)
end

grammar_dg.TryFinish = class.class('DGTryFinish', grammar_dg.Control)

function grammar_dg.TryFinish.method:__init(name)
	class.super(grammar_dg.TryFinish).__init(self)
	self.name = name
end

function grammar_dg.TryFinish.method:_apply(ctx)
	if not self.name then
		-- Merge unnamed temp result ctx with parent result context
		local res = ctx:result()
		ctx:pop()

		local src = rawget(res, '_validate')
		if src then
			local dst = rawget(ctx:result(), '_validate')
			if not dst then
				rawset(ctx:result(), '_validate', rawget(res, '_validate'))
			else
				table.merge(dst, src)
			end
			rawset(res, '_validate', nil)
		end

		class.merge(ctx:result(), res)
	else
		ctx:pop()
	end

	ctx:popcatch()
end

grammar_dg.ArrayStart = class.class('DGArrayStart', grammar_dg.Control)

grammar_dg.ArrayStart.trace_name = 'array'

function grammar_dg.ArrayStart.method:__init(rule, id, name, entity, create, resultclass)
	class.super(grammar_dg.ArrayStart).__init(self, rule, id)
	self.name = name
	self.entity = entity
	self.create = create
	self.resultclass = resultclass or grammar.ArrayResult
end

function grammar_dg.ArrayStart.method:_apply(ctx)
	local res = ctx:result()
	if self.name then
		local result = self.resultclass:new()
		result:_init(ctx.iter:copy(), self.entity, self.create)
		local array = ctx:push(result, self.name)

		if self.converter then
			res:addproperty(self.name,
				function (this) return self.converter.get(array) end
			)
		else
			res[self.name] = array
		end
	else
		ctx:push({}, nil)
	end
end

grammar_dg.ArrayFinish = class.class('DGArrayFinish', grammar_dg.Control)

function grammar_dg.ArrayFinish.method:__init()
	class.super(grammar_dg.ArrayFinish).__init(self)
end

function grammar_dg.ArrayFinish.method:apply(ctx)
	ctx:pop()
end

grammar_dg.ArrayPush = class.class('DGArrayPush', grammar_dg.Control)

function grammar_dg.ArrayPush.method:__init(rule, id)
	class.super(grammar_dg.ArrayPush).__init(self, rule, id)
end

function grammar_dg.ArrayPush.method:_apply(ctx)
	self:trace(ctx.iter, "append array element")

	local res = ctx:result()
	if class.isa(res, grammar.ArrayResult) then
		rawset(res, '_entitybegin', ctx.iter:copy())
	end
	local new = ctx:push(nil, #res+1)
	table.insert(res, new)
end

grammar_dg.ArrayPop = class.class('DGArrayPop', grammar_dg.Control)

function grammar_dg.ArrayPop.method:__init()
	class.super(grammar_dg.ArrayPop).__init(self)
end

function grammar_dg.ArrayPop.method:_apply(ctx)
	local entityresult = ctx:result()
	ctx:pop()
	local arrayresult = ctx:result()
	if class.isa(arrayresult, grammar.ArrayResult) then
		rawset(entityresult, '_sub', haka.vbuffer_sub(arrayresult._entitybegin, ctx.iter))
		rawset(arrayresult, '_entitybegin', nil)
		ctx.iter:split()
	end
end

grammar_dg.Error = class.class('DGError', grammar_dg.Control)

function grammar_dg.Error.method:__init(id, msg)
	class.super(grammar_dg.Error).__init(self, nil, id)
	self.msg = msg
end

function grammar_dg.Error.method:_dump_graph_options()
	return ',fillcolor="#ee0000",style="filled"'
end

function grammar_dg.Error.method:_apply(ctx)
	return ctx:error(nil, self, self.msg)
end

grammar_dg.Execute = class.class('DGExecute', grammar_dg.Control)

function grammar_dg.Execute.method:__init(rule, id, callback)
	class.super(grammar_dg.Error).__init(self, rule, id)
	self.callback = callback
end

function grammar_dg.Execute.method:_apply(ctx)
	self.callback(ctx:result(), ctx)
end

grammar_dg.Retain = class.class('DGRetain', grammar_dg.Control)

function grammar_dg.Retain.method:__init(readonly)
	class.super(grammar_dg.Error).__init(self)
	self.readonly = readonly
end

function grammar_dg.Retain.method:_apply(ctx)
	-- We try to mark the incoming data, so wait for them
	-- to arrive before marking the end of a previous chunk
	ctx.iter:wait()

	ctx:mark(self.readonly)
end

grammar_dg.Release = class.class('DGRelease', grammar_dg.Control)

function grammar_dg.Release.method:_apply(ctx)
	ctx:unmark()
end

grammar_dg.Branch = class.class('DGBranch', grammar_dg.Control)

function grammar_dg.Branch.method:__init(rule, id, selector)
	class.super(grammar_dg.Branch).__init(self, rule, id)
	self.selector = selector
	self.cases = {}
end

function grammar_dg.Branch.method:case(key, entity)
	self.cases[key] = entity
end

function grammar_dg.Branch.method:_dump_graph_edges(file, ref)
	for name, case in pairs(self.cases) do
		case:_dump_graph(file, ref)
		file:write(string.format('%s -> %s [label="%s"];\n', ref[self], ref[case], name))
	end

	if self._next then
		self._next:_dump_graph(file, ref)
		file:write(string.format('%s -> %s [label="default"];\n', ref[self], ref[self._next]))
	end
end

function grammar_dg.Branch.method:next(ctx)
	local case = self.selector(ctx:result(), ctx)
	self:trace(ctx.iter, "select branch '%s'", case)

	local next = self.cases[case]

	if next then return next
	else return self._next end
end

function grammar_dg.Branch.method:_nexts(list)
	for _, value in pairs(self.cases) do
		table.insert(list, value)
	end

	return class.super(grammar_dg.Branch)._nexts(self, list)
end

grammar_dg.Primitive = class.class('DGPrimitive', grammar_dg.Entity)

function grammar_dg.Primitive.method:_apply(ctx)
	return self:_parse(ctx:result(), ctx.iter, ctx)
end

function grammar_dg.Primitive.method:_create(ctx)
	return self:_init(ctx:result(), ctx.iter, ctx, ctx.current_init)
end

grammar_dg.Number = class.class('DGNumber', grammar_dg.Primitive)

grammar_dg.Number.trace_name = 'number'

function grammar_dg.Number.method:__init(rule, id, size, endian, name)
	class.super(grammar_dg.Number).__init(self, rule, id)
	self.size = size
	self.endian = endian
	self.name = name
end

function grammar_dg.Number.method:_dump_graph_descr()
	return string.format("%d bits (%s endian)", self.size, self.endian or 'big')
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

grammar_dg.Bits = class.class('DGBits', grammar_dg.Primitive)

function grammar_dg.Bits.method:__init(rule, id, size)
	class.super(grammar_dg.Bits).__init(self, rule, id)
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

grammar_dg.Bytes = class.class('DGBytes', grammar_dg.Primitive)

function grammar_dg.Bytes.method:__init(rule, id, size, name, chunked_callback)
	class.super(grammar_dg.Bytes).__init(self, rule, id)
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
			-- Call the callback to notify for the stream end
			self.chunked_callback(res, nil, true, ctx)
		else
			while (size == 'all' or size > 0) and iter:wait() do
				if size ~= 'all' then
					local ret, subsize = iter:check_available(size, true)
					if ret then
						sub = iter:sub(size, true)
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

grammar_dg.Token = class.class('DGToken', grammar_dg.Primitive)

grammar_dg.Token.trace_name = 'token'

function grammar_dg.Token.method:__init(rule, id, pattern, re, name)
	class.super(grammar_dg.Token).__init(self, rule, id)
	self.pattern = pattern
	self.re = re
	self.name = name
end

function grammar_dg.Token.method:_dump_graph_descr()
	return string.format("/%s/", safe_string(self.pattern))
end

function grammar_dg.Token.method:_parse(res, iter, ctx)
	local sink = self.re:create_sink()
	local begin

	while true do
		iter:wait()

		if not begin then
			begin = iter:copy()
			begin:mark(haka.packet_mode() == 'passthrough')
		end

		local sub = iter:sub('available')

		local match, mbegin, mend = sink:feed(sub, sub == nil)
		if not match and not sink:ispartial() then break end

		if match then
			begin:unmark()

			if mend then iter:move_to(mend)
			else mend = iter end

			iter:split()

			local result = haka.vbuffer_sub(begin, mend)
			local string = result:asstring()

			if self.converter then
				string = self.converter.get(string)
			end

			if self.name then
				res:addproperty(self.name,
					function (this)
						return string
					end,
					function (this, newvalue)
						string = newvalue
						if self.converter then
							newvalue = self.converter.set(newvalue)
						end

						if not self.re:match(newvalue) then
							error(string.format("token value '%s' does not verify /%s/", newvalue, self.pattern))
						end

						result:setstring(newvalue)
					end
				)
			end
			sink = nil
			return
		end

		if not sub then break end
	end

	-- No match found return an error
	if begin then
		begin:unmark()
	end
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
grammar_int.converter = grammar.converter

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

grammar.converter.string = {
	get = function (x) return x:asstring() end,
	set = function (x) return x end
}


--
-- Grammar description
--

grammar_int.Entity = class.class('Entity')

function grammar_int.Entity.method:_as(name)
	local clone = self:clone()
	clone.named = name
	return clone
end

function grammar_int.Entity.method:convert(converter, memoize)
	local clone = self:clone()
	clone.converter = converter
	clone.memoize = memoize or clone.memoize
	return clone
end

function grammar_int.Entity.method:validate(validate)
	local clone = self:clone()
	clone.validate = validate
	return clone
end

function grammar_int.Entity.getoption(cls, opt)
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

function grammar_int.Entity.method:options(options)
	local clone = self:clone()
	for k, v in pairs(options) do
		if type(k) == 'string' then
			local opt = grammar_int.Entity.getoption(class.classof(self), k)
			assert(opt, string.format("invalid option '%s'", k))
			opt(clone, v)
		elseif type(v) == 'string' then
			local opt = grammar_int.Entity.getoption(class.classof(self), v)
			assert(opt, string.format("invalid option '%s'", v))
			opt(clone)
		else
			error("invalid option")
		end
	end
	return clone
end

grammar_int.Entity._options = {}

function grammar_int.Entity._options.memoize(self)
	self.memoize = true
end


grammar_int.Record = class.class('Record', grammar_int.Entity)

function grammar_int.Record.method:__init(entities)
	self.entities = entities
	self.extra_entities = {}
	self.on_finish = {}
end

function grammar_int.Record.method:extra(functions)
	for name, func in pairs(functions) do
		if type(name) == 'string' then self.extra_entities[name] = func
		else table.insert(self.on_finish, func) end
	end
	return self
end

function grammar_int.Record.method:compile(rule, id)
	local iter, ret

	ret = grammar_dg.Retain:new(haka.packet_mode() == 'passthrough')

	iter = grammar_dg.RecordStart:new(rule, id, self.named)
	if self.converter then iter:convert(self.converter, self.memoize) end
	if self.validate then iter:validate(self.validate) end
	ret:add(iter)

	for i, entity in ipairs(self.entities) do
		local next = entity:compile(self.rule or rule, i)
		if next then
			if iter then
				iter:add(next)
				iter = next
			else
				ret = next
				iter = ret
			end
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
	iter:add(grammar_dg.Release:new())

	return ret
end


grammar_int.Sequence = class.class('Sequence', grammar_int.Entity)

function grammar_int.Sequence.method:__init(entities)
	self.entities = entities
end

function grammar_int.Sequence.method:compile(rule, id)
	local iter, ret

	ret = grammar_dg.RecordStart:new(rule, id, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	if self.validate then ret:validate(self.validate) end
	iter = ret

	for i, entity in ipairs(self.entities) do
		if entity.named then
			haka.log.warning("grammar", "named element '%s' are not supported in sequence", entity.named)
		end

		local next = entity:compile(self.rule or rule, i)
		if next then
			if iter then
				iter:add(next)
				iter = next
			else
				ret = next
				iter = ret
			end
		end
	end

	local pop = grammar_dg.RecordFinish:new(self.named ~= nil)
	iter:add(pop)

	return ret
end


grammar_int.Union = class.class('Union', grammar_int.Entity)

function grammar_int.Union.method:__init(entities)
	self.entities = entities
end

function grammar_int.Union.method:compile(rule, id)
	local ret = grammar_dg.UnionStart:new(rule, id, self.named)

	for i, value in ipairs(self.entities) do
		local next = value:compile(self.rule or rule, i)
		if next then
			ret:add(next)
			ret:add(grammar_dg.UnionRestart:new())
		end
	end

	ret:add(grammar_dg.UnionFinish:new(self.named))
	return ret
end

grammar_int.Try = class.class('Try', grammar.Entity)

function grammar_int.Try.method:__init(cases)
	self.cases = cases
end

function grammar_int.Try.method:compile(rule, id)
	local first_try = nil
	local previous_try = nil
	local finish = grammar_dg.TryFinish:new(self.rule or rule, id, self.named)

	-- For each case we prepend a catch entity
	-- and we chain catch entities together to try every cases
	for i, case in ipairs(self.cases) do
		local next = case:compile(self.rule or rule, i)
		if next then
			local try = grammar_dg.Try:new(self.rule or rule, id, self.named)
			try:add(next)
			try:add(finish)
			if previous_try then
				previous_try:catch(try)
			end
			previous_try = try
			first_try = first_try or try
		end
	end

	if not first_try then
		error("cannot create a try without any case")
	end

	-- End with error if everything failed
	previous_try:catch(grammar_dg.Error:new(self.rule or rule, "no case successfull"))

	return first_try
end

grammar_int.Branch = class.class('Branch', grammar_int.Entity)

function grammar_int.Branch.method:__init(cases, select)
	self.selector = select
	self.cases = { default = 'error' }
	for key, value in pairs(cases) do
		self.cases[key] = value
	end
end

function grammar_int.Branch.method:compile(rule, id)
	local ret = grammar_dg.Branch:new(rule, id, self.selector)
	for key, value in pairs(self.cases) do
		if key ~= 'default' then
			local next = value:compile(self.rule or rule, key)
			if next then
				ret:case(key, next)
			end
		end
	end

	local default = self.cases.default
	if default == 'error' then
		ret._next = grammar_dg.Error:new(self.rule or rule, "invalid case")
	elseif default ~= 'continue' then
		local next = default:compile(self.rule or rule, 'default')
		if next then
			ret._next = next
		end
	end

	return ret
end


grammar_int.Array = class.class('Array', grammar_int.Entity)

function grammar_int.Array.method:__init(entity)
	self.entity = entity
end

function grammar_int.Array.method:compile(rule, id)
	local entity = self.entity:compile(self.rule or rule, id)
	if not entity then
		error("cannot create an array of empty element")
	end
	local start = grammar_dg.ArrayStart:new(rule, id, self.named, entity, self.create, self.resultclass)
	if self.converter then start:convert(self.converter, self.memoize) end
	local finish = grammar_dg.ResultPop:new()

	local pop = grammar_dg.ArrayPop:new()
	local inner = grammar_dg.ArrayPush:new(rule, id)
	inner:add(self.entity:compile(self.rule or rule, id))
	inner:add(pop)

	local loop = grammar_dg.Branch:new(nil, nil, self.more)
	pop:add(loop)
	loop:case(true, inner)
	loop:add(finish)
	start:add(loop)

	return start
end

grammar_int.Array._options = {}

function grammar_int.Array._options.count(self, size)
	local sizefunc

	if type(size) ~= 'function' then
		sizefunc = function () return size end
	else
		sizefunc = size
	end

	self.more = function (array, ctx)
		return #array < sizefunc(ctx:result(-2), ctx)
	end
end

function grammar_int.Array._options.untilcond(self, condition)
	self.more = function (array, ctx)
		if #array == 0 then return not condition(nil, ctx)
		else return not condition(array[#array], ctx) end
	end
end

function grammar_int.Array._options.whilecond(self, condition)
	self.more = function (array, ctx)
		if #array == 0 then return condition(nil, ctx)
		else return condition(array[#array], ctx) end
	end
end

function grammar_int.Array._options.create(self, f)
	self.create = f
end

function grammar_int.Array._options.result(self, resultclass)
	self.resultclass = resultclass
end


grammar_int.Number = class.class('Number', grammar_int.Entity)

function grammar_int.Number.method:__init(bits)
	self.bits = bits
end

function grammar_int.Number.method:compile(rule, id)
	local ret = grammar_dg.Number:new(rule, id, self.bits, self.endian, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	if self.validate then ret:validate(self.validate) end
	return ret
end

grammar_int.Number._options = {}
function grammar_int.Number._options.endianness(self, endian) self.endian = endian end


grammar_int.Bytes = class.class('Bytes', grammar_int.Entity)

function grammar_int.Bytes.method:compile(rule, id)
	if type(self.count) ~= 'function' then
		local count = self.count
		self.count = function (self) return count end
	end

	local ret = grammar_dg.Bytes:new(rule, id, self.count, self.named, self.chunked)
	if self.converter then ret:convert(self.converter, self.memoize) end
	if self.validate then ret:validate(self.validate) end
	return ret
end

grammar_int.Bytes._options = {}
function grammar_int.Bytes._options.chunked(self, callback) self.chunked = callback end
function grammar_int.Bytes._options.count(self, count) self.count = count end


grammar_int.Bits = class.class('Bits', grammar_int.Entity)

function grammar_int.Bits.method:__init(bits)
	self.bits = bits
end

function grammar_int.Bits.method:compile(rule, id)
	if type(self.bits) ~= 'function' then
		local bits = self.bits
		self.bits = function (self) return bits end
	end

	return grammar_dg.Bits:new(rule, id, self.bits)
end

grammar_int.Token = class.class('Token', grammar_int.Entity)

function grammar_int.Token.method:__init(pattern)
	self.pattern = pattern
end

function grammar_int.Token.method:compile(rule, id)
	if not self.re then
		self.re = rem.re:compile("^(?:"..self.pattern..")")
	end
	local ret = grammar_dg.Token:new(rule, id, self.pattern, self.re, self.named)
	if self.converter then ret:convert(self.converter, self.memoize) end
	return ret
end

grammar_int.Execute = class.class('Execute', grammar_int.Entity)

function grammar_int.Execute.method:__init(func)
	self.func = func
end

function grammar_int.Execute.method:compile(rule, id)
	return grammar_dg.Execute:new(rule, id, self.func)
end

grammar_int.Retain = class.class('Retain', grammar_int.Entity)

function grammar_int.Retain.method:__init(readonly)
	self.readonly = readonly
end

function grammar_int.Retain.method:compile(rule, id)
	return grammar_dg.Retain:new(self.readonly)
end

grammar_int.Release = class.class('Release', grammar_int.Entity)

function grammar_int.Release.method:compile(rule, id)
	return grammar_dg.Release:new()
end

grammar_int.Empty = class.class('Empty', grammar_int.Entity)

function grammar_int.Empty.method:compile(rule, id)
	return nil
end

grammar_int.Error = class.class('Error', grammar_int.Entity)

function grammar_int.Error.method:__init(msg)
	self.msg = msg
end

function grammar_int.Error.method:compile(rule, id)
	return grammar_dg.Error:new(id, self.msg)
end

function grammar_int.record(entities)
	return grammar_int.Record:new(entities)
end

function grammar_int.sequence(entities)
	return grammar_int.Sequence:new(entities)
end

function grammar_int.union(entities)
	return grammar_int.Union:new(entities)
end

function grammar_int.try(cases)
	return grammar_int.Try:new(cases)
end

function grammar_int.branch(cases, select)
	return grammar_int.Branch:new(cases, select)
end

function grammar_int.optional(entity, present)
	return grammar_int.Branch:new({
		[true] = entity,
		default = 'continue'
	}, present)
end

function grammar_int.array(entity)
	return grammar_int.Array:new(entity)
end

function grammar_int.number(bits)
	return grammar_int.Number:new(bits)
end

function grammar_int.token(pattern)
	return grammar_int.Token:new(pattern)
end

grammar_int.flag = grammar_int.number(1):convert(grammar_int.converter.bool, false)

function grammar_int.bytes()
	return grammar_int.Bytes:new()
end

function grammar_int.padding(args)
	if args.align then
		local align = args.align
		return grammar_int.Bits:new(function (self, ctx)
			local rem = (ctx.iter.meter * 8 + ctx._bitoffset) % align
			if rem > 0 then return align -rem
			else return 0 end
		end)
	elseif args.size then
		return grammar_int.Bits:new(args.size)
	else
		error("invalid padding option")
	end
end

function grammar_int.field(name, field)
	return field:_as(name)
end

function grammar_int.verify(func, msg)
	return grammar_int.Execute:new(function (self, ctx)
		if not func(self, ctx) then
			error(msg)
		end
	end)
end

function grammar_int.execute(func)
	return grammar_int.Execute:new(func)
end

function grammar_int.empty()
	return grammar_int.Empty:new()
end

function grammar_int.error(msg)
	return grammar_int.Error:new(msg)
end

grammar_int.text = grammar_int.bytes():convert(grammar_int.converter.string, true)

--
-- Grammar
--

local Grammar = class.class("Grammar")

function Grammar.method:__init(name)
	rawset(self, '_name', name)
	rawset(self, '_rules', {})
	rawset(self, '_exports', {})
end

function Grammar.method:dump_graph(file)
	file:write("digraph grammar {\n")

	local ref = {}
	ref._index = 1

	for name, rule in pairs(self._exports) do
		file:write(string.format("subgraph cluster_%s { label=%s \n", name, name))
		rule:_dump_graph(file, ref)
		file:write("}\n")
	end

	file:write("}\n")
end

function Grammar.method:__index(name)
	local ret = self._exports[name]
	if not ret then
		error(string.format("unknown grammar public rule '%s'", name))
	end

	return ret
end

function Grammar.method:__newindex(key, value)
	error("read-only table")
end


local GrammarEnv = class.class("GrammarEnv")

GrammarEnv.dump_graph = false

function GrammarEnv.method:__init(res, env)
	rawset(self, '_res', res)
	rawset(self, '_env', env)
	
	self.export = function (...)
		local rules = table.invert(self._res._rules)

		for _, value in ipairs({...}) do
			local name = rules[value]
			if not name then
				error("exported rule must be registered in the grammar")
			end

			self._res._exports[name] = value:compile()
		end
	end
end

function GrammarEnv.method:__index(name)
	local ret

	-- Search in the grammar environment
	ret = grammar_int[name]
	if ret then return ret end

	-- Search the defined rules
	ret = self._res._rules[name]
	if ret then return ret end

	-- Or in the global environment
	return self._env[name]
end

function GrammarEnv.method:__newindex(key, value)
	-- Forbid override to grammar elements
	if grammar[name] then
		error(string.format("'%s' is reserved in the grammar scope", key))
	end

	-- Add the object in the rules
	self._res._rules[key] = value

	if class.isa(value, grammar_int.Entity) then
		value.rule = key
	end
end

function grammar.new(name, def)
	assert(type(def) == 'function', "grammar definition must by a function")

	local g = Grammar:new(name)
	local env = GrammarEnv:new(g, debug.getfenv(def))
	debug.setfenv(def, env)

	def()

	if grammar.debug then
		haka.log.warning("grammar", "dumping '%s' grammar graph to %s.dot", g._name, g._name)
		f = io.open(string.format("%s.dot", g._name), "w+")
		g:dump_graph(f)
		f:close()
	end

	return g
end

grammar.debug = false

haka.grammar = grammar

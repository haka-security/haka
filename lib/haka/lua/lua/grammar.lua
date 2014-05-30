-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local rem = require("regexp/pcre")
local parse = require("parse")
local grammar_dg = require('grammar_dg')

local grammar_int = {}
local grammar = {}

grammar.result = require("parse_result")

--
-- Grammar env
--

local GrammarEnv = class.class("GrammarEnv")

function GrammarEnv.method:__init(grammar)
	self._grammar = grammar
	self._compiled = {}
end

function GrammarEnv.method:get(entity)
	return self._compiled[entity]
end

function GrammarEnv.method:register(entity, dg)
	self._compiled[entity] = dg
end

function GrammarEnv.method:unregister(entity, dg)
	self._compiled[entity] = nil
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

function grammar_int.Entity.method:compile(env, rule, id)
	return self:do_compile(env, rule, id)
end

function grammar_int.Entity.method:convert(converter, memoize)
	local clone = self:clone()
	clone._converter = converter
	clone._memoize = memoize or clone._memoize
	return clone
end

function grammar_int.Entity.method:validate(validate)
	local clone = self:clone()
	clone._validate = validate
	return clone
end

function grammar_int.Entity.method:memoize()
	local clone = self:clone()
	clone._memoize = true
	return clone
end

grammar_int.Compound = class.class('Compound', grammar_int.Entity)

function grammar_int.Compound.method:compile(env, rule, id)
	local compiled = env:get(self)
	if compiled then
		return grammar_dg.Recurs:new(rule, id, compiled)
	end

	-- Create a DGCompound in order to use it for recursive case
	local compound = grammar_dg.Compound:new(rule, id, compiled)
	env:register(self, compound)
	local ret = self:do_compile(env, rule, id)
	env:unregister(self)
	compound:add(ret)
	return compound
end


grammar_int.Record = class.class('Record', grammar_int.Compound)

function grammar_int.Record.method:__init(entities)
	self.entities = entities
	self.extra_entities = {}
end

function grammar_int.Record.method:extra(functions)
	for name, func in pairs(functions) do
		if type(name) ~= 'string' then
			error("record extra need to be named")
		end
		
		self.extra_entities[name] = func
	end

	return self
end

function grammar_int.Record.method:do_compile(env, rule, id)
	local iter, ret

	ret = grammar_dg.Retain:new(haka.packet_mode() == 'passthrough')

	iter = grammar_dg.RecordStart:new(rule, id, self.named)
	if self._converter then iter:convert(self._converter, self._memoize) end
	if self._validate then iter:validate(self._validate) end
	ret:add(iter)

	for i, entity in ipairs(self.entities) do
		local next = entity:compile(env, self.rule or rule, i)
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
	for name, f in pairs(self.extra_entities) do
		pop:extra(name, f)
	end
	iter:add(pop)
	iter:add(grammar_dg.Release:new())

	return ret
end


grammar_int.Sequence = class.class('Sequence', grammar_int.Compound)

function grammar_int.Sequence.method:__init(entities)
	self.entities = entities
end

function grammar_int.Sequence.method:do_compile(env, rule, id)
	local iter, ret

	ret = grammar_dg.RecordStart:new(rule, id, self.named)

	if self._converter then ret:convert(self._converter, self._memoize) end
	if self._validate then ret:validate(self._validate) end
	iter = ret

	for i, entity in ipairs(self.entities) do
		if entity.named then
			haka.log.warning("grammar", "named element '%s' are not supported in sequence", entity.named)
		end

		local next = entity:compile(env, self.rule or rule, i)
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


grammar_int.Union = class.class('Union', grammar_int.Compound)

function grammar_int.Union.method:__init(entities)
	self.entities = entities
end

function grammar_int.Union.method:do_compile(env, rule, id)
	local ret = grammar_dg.UnionStart:new(rule, id, self.named)

	for i, entity in ipairs(self.entities) do
		local next = entity:compile(env, self.rule or rule, i)
		if next then
			ret:add(next)
			ret:add(grammar_dg.UnionRestart:new())
		end
	end

	ret:add(grammar_dg.UnionFinish:new(self.named))
	return ret
end

grammar_int.Try = class.class('Try', grammar_int.Compound)

function grammar_int.Try.method:__init(cases)
	self.cases = cases
end

function grammar_int.Try.method:do_compile(env, rule, id)
	local first_try = nil
	local previous_try = nil
	local finish = grammar_dg.TryFinish:new(self.rule or rule, id, self.named)

	-- For each case we prepend a catch entity
	-- and we chain catch entities together to try every cases
	for i, entity in ipairs(self.cases) do
		local next = entity:compile(env, self.rule or rule, i)

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
		error("cannot create a try element without any case")
	end

	-- End with error if everything failed
	previous_try:catch(grammar_dg.Error:new(self.rule or rule, "cannot find a successful try case"))

	return first_try
end

grammar_int.Branch = class.class('Branch', grammar_int.Compound)

function grammar_int.Branch.method:__init(cases, select)
	self.selector = select
	self.cases = { default = 'error' }
	for key, value in pairs(cases) do
		self.cases[key] = value
	end
end

function grammar_int.Branch.method:do_compile(env, rule, id)
	local ret = grammar_dg.Branch:new(rule, id, self.selector)

	for key, entity in pairs(self.cases) do
		if key ~= 'default' then
			local next = entity:compile(env, self.rule or rule, key)
			if next then
				ret:case(key, next)
			end
		end
	end

	local default = self.cases.default
	if default == 'error' then
		ret._next = grammar_dg.Error:new(self.rule or rule, "invalid case")
	elseif default ~= 'continue' then
		local next = default:compile(env, self.rule or rule, 'default')
		if next then
			ret._next = next
		end
	end

	return ret
end


grammar_int.Array = class.class('Array', grammar_int.Compound)

function grammar_int.Array.method:__init(entity)
	self.entity = entity
end


function grammar_int.Array.method:do_compile(env, rule, id)
	local start = grammar_dg.ArrayStart:new(rule, id, self.named, self.create, self.resultclass)
	if self._converter then start:convert(self._converter, self._memoize) end

	local loop = grammar_dg.Branch:new(nil, nil, self.more)
	local push = grammar_dg.ArrayPush:new(rule, id)
	local inner = self.entity:compile(env, self.rule or rule, id)
	local pop = grammar_dg.ArrayPop:new()
	local finish = grammar_dg.ResultPop:new()

	start:set_entity(inner)

	push:add(inner)
	push:add(pop)
	push:add(loop)
	loop:case(true, push)
	loop:add(finish)
	start:add(loop)

	return start
end

function grammar_int.Array.method:count(size)
	local clone = self:clone()

	local sizefunc

	if type(size) ~= 'function' then
		sizefunc = function () return size end
	else
		sizefunc = size
	end

	clone.more = function (array, ctx)
		return #array < sizefunc(ctx:result(-2), ctx)
	end

	return clone
end

function grammar_int.Array.method:untilcond(condition)
	local clone = self:clone()

	clone.more = function (array, ctx)
		if #array == 0 then return not condition(nil, ctx)
		else return not condition(array[#array], ctx) end
	end

	return clone
end

function grammar_int.Array.method:whilecond(condition)
	local clone = self:clone()

	self.more = function (array, ctx)
		if #array == 0 then return condition(nil, ctx)
		else return condition(array[#array], ctx) end
	end

	return clone
end

function grammar_int.Array.method:creation(f)
	local clone = self:clone()
	clone.create = f
	return clone
end

function grammar_int.Array.method:result(resultclass)
	local clone = self:clone()
	clone.resultclass = resultclass
	return clone
end


grammar_int.Number = class.class('Number', grammar_int.Entity)

function grammar_int.Number.method:__init(bits, endian)
	self.bits = bits
	self.endian = endian or 'big'
end

function grammar_int.Number.method:do_compile(env, rule, id)
	local ret = grammar_dg.Number:new(rule, id, self.bits, self.endian, self.named)
	if self._converter then ret:convert(self._converter, self._memoize) end
	if self._validate then ret:validate(self._validate) end
	return ret
end


grammar_int.Bytes = class.class('Bytes', grammar_int.Entity)

function grammar_int.Bytes.method:do_compile(env, rule, id)
	local ret = grammar_dg.Bytes:new(rule, id, self._count, self.named, self._chunked)
	if self._converter then ret:convert(self._converter, self._memoize) end
	if self._validate then ret:validate(self._validate) end
	return ret
end

function grammar_int.Bytes.method:chunked(callback)
	local clone = self:clone()
	clone._chunked = callback
	return clone
end

function grammar_int.Bytes.method:count(count)
	local clone = self:clone()

	if type(count) ~= 'function' then
		clone._count = function (self) return count end
	else
		clone._count = count
	end

	return clone
end


grammar_int.Bits = class.class('Bits', grammar_int.Entity)

function grammar_int.Bits.method:__init(bits)
	self.bits = bits
end

function grammar_int.Bits.method:do_compile(env, rule, id)
	if type(self.bits) ~= 'function' then
		local bits = self.bits
		self.bits = function (self) return bits end
	end

	return grammar_dg.Bits:new(rule, id, self.bits)
end

grammar_int.Token = class.class('Token', grammar_int.Entity)

function grammar_int.Token.method:__init(pattern, raw)
	self.pattern = pattern
	self.raw = raw
end

function grammar_int.Token.method:do_compile(env, rule, id)
	if not self.re then
		self.re = rem.re:compile("^(?:"..self.pattern..")")
	end
	local ret = grammar_dg.Token:new(rule, id, self.pattern, self.re, self.named, self.raw)
	if self._converter then ret:convert(self._converter, self._memoize) end
	return ret
end

grammar_int.Execute = class.class('Execute', grammar_int.Entity)

function grammar_int.Execute.method:__init(func)
	self.func = func
end

function grammar_int.Execute.method:do_compile(env, rule, id)
	return grammar_dg.Execute:new(rule, id, self.func)
end

grammar_int.Retain = class.class('Retain', grammar_int.Entity)

function grammar_int.Retain.method:__init(readonly)
	self.readonly = readonly
end

function grammar_int.Retain.method:do_compile(env, rule, id)
	return grammar_dg.Retain:new(self.readonly)
end

grammar_int.Release = class.class('Release', grammar_int.Entity)

function grammar_int.Release.method:do_compile(env, rule, id)
	return grammar_dg.Release:new()
end

grammar_int.Empty = class.class('Empty', grammar_int.Entity)

function grammar_int.Empty.method:do_compile(env, rule, id)
	return nil
end

grammar_int.Error = class.class('Error', grammar_int.Entity)

function grammar_int.Error.method:__init(msg)
	self.msg = msg
end

function grammar_int.Error.method:do_compile(env, rule, id)
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
	return grammar_int.Token:new(pattern, false)
end

function grammar_int.raw_token(pattern)
	return grammar_int.Token:new(pattern, true)
end

grammar_int.flag = grammar_int.number(1):convert(grammar_int.converter.bool, false)

function grammar_int.bytes()
	return grammar_int.Bytes:new()
end

function grammar_int.align(size)
	return grammar_int.Bits:new(function (self, ctx)
		local rem = (ctx.iter.meter * 8 + ctx._bitoffset) % align
		if rem > 0 then return size -rem
		else return 0 end
	end)
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

function grammar_int.fail(msg)
	return grammar_int.Error:new(msg)
end

grammar_int.text = grammar_int.bytes():convert(grammar_int.converter.string, true)

local GrammarProxy = class.class("GrammarProxy", grammar_int.Entity)

function GrammarProxy.method:__init(target)
	self._target = target
end

function GrammarProxy.method:do_compile(env, rule, id)
	local entity = env._grammar._rules[self._target]
	if not entity then
		error("use of unimplemented entity: %s", proxy._target)
	end

	if self.named then
		local clone = entity:clone()
		clone.named = self.named
		entity = clone
	end

	return entity:compile(env, rule, id)
end


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

function Grammar.method:export(...)
	for _, proxy in ipairs({...}) do
		if not class.isa(proxy, GrammarProxy) then
			error("can only export named rules")
		end
		self._exports[proxy._target] = true
	end
end

function Grammar.method:extend(...)
	for _, grammar in ipairs({...}) do
		table.merge(self._rules, grammar._rules)
		-- We just need key of exports to recompile it
		table.merge(self._exports, grammar._exports)
	end
end

function Grammar.method:define(...)
	for _, name in ipairs({...}) do
		self._rules[name] = GrammarProxy:new(name)
	end
end

local function grammar_env(gr)
	grammar_int.export = function (...) gr:export(...) end
	grammar_int.extend = function (...) gr:extend(...) end
	grammar_int.define = function (...) gr:define(...) end

	return {
		__index = function (self, name)
			local ret

			-- Search in the grammar environment
			ret = grammar_int[name]
			if ret then return ret end

			-- Search the defined rules
			ret = gr._rules[name]
			if ret then
				if class.isa(ret, GrammarProxy) and ret._target == name then
					return ret
				else
					-- Create a proxy to allow inheritance
					return GrammarProxy:new(name)
				end
			end

			return nil
		end;
		__newindex = function (self, key, value)
			-- Forbid override to grammar elements
			if grammar_int[key] then
				error(string.format("'%s' is reserved in the grammar scope", key))
			end

			if class.isa(value, grammar_int.Entity) then
				-- Add the object in the rules
				gr._rules[key] = value
				value.rule = key
			else
				rawset(self, key, value)
			end
		end
	}
end

function grammar.new(name, def)
	assert(type(def) == 'function', "grammar definition must be a function")

	local g = Grammar:new(name)

	-- Add a metatable to the environment only during the definition
	-- of the grammar.
	local env = debug.getfenv(def)
	setmetatable(env, grammar_env(g))

	def()
	setmetatable(env, nil)

	-- Compile exported entities
	for name, _  in pairs(g._exports) do
		local value = g._rules[name]
		if not value then
			error("exported rule must be registered in the grammar: "..name)
		end

		local genv = GrammarEnv:new(g)
		g._exports[name] = value:compile(genv)
	end

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

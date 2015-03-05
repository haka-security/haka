-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local rem = require("regexp/pcre")
local parse = require("parse")
local grammar_dg = require('grammar_dg')
local ccomp = require('ccomp')

local grammar_int = {}
local grammar = {}
local log = haka.log_section("grammar")

grammar.result = require("parse_result")

--
-- Grammar env
--

local GrammarEnv = class.class("GrammarEnv")

function GrammarEnv.method:__init(grammar)
	self._grammar = grammar
	self._compiled = {}
	-- gid stand for global id
	-- it represent a unique id for node in a given grammar
	self._gid = 0
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

function GrammarEnv.method:gid()
	self._gid = self._gid + 1
	return self._gid
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
-- Grammar description classes
--

grammar_int.Entity = class.class('Entity')

function grammar_int.Entity.method:clone()
	local clone = class.super(grammar_int.Entity).clone(self)
	if self._apply then clone._apply = table.copy(self._apply) end
	return clone
end

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

function grammar_int.Entity.method:apply(apply)
	local clone = self:clone()
	if not clone._apply then clone._apply = {} end
	table.insert(clone._apply, apply)
	return clone
end

function grammar_int.Entity.method:const(value)
	local apply

	if type(value) ~= 'function' then
		apply = function (v, res, ctx)
			if v ~= value then
				ctx:error("incorrect value, expected '%s' got '%s'", value, v)
			end
		end
	else
		apply = function (v, res, ctx)
			local ref = value(res, ctx)
			if v ~= ref then
				ctx:error("incorrect value, expected '%s' got '%s'", ref, v)
			end
		end
	end

	return self:apply(apply)
end

function grammar_int.Entity.method:property_setup(item)
	if self._converter or self._memoize then item:convert(self._converter, self._memoize) end
	if self._validate then item:validate(self._validate) end
end

function grammar_int.Entity.method:post_setup(item)
	if self._apply then
		for _, apply in ipairs(self._apply) do
			item:add_apply(apply)
		end
	end
end

function grammar_int.Entity.method:compile_setup(item)
	self:property_setup(item)
	self:post_setup(item)
end

grammar_int.Compound = class.class('Compound', grammar_int.Entity)

function grammar_int.Compound.method:result(resultclass)
	local clone = self:clone()
	clone.resultclass = resultclass
	return clone
end

function grammar_int.Compound.method:compile(env, rule, id)
	local compiled = env:get(self)
	if compiled then
		return grammar_dg.Recurs:new(env:gid(), rule, id, compiled)
	end

	-- Create a DGCompound in order to use it for recursive case
	local compound = grammar_dg.CompoundStart:new(env:gid(), rule, id, self.resultclass)
	env:register(self, compound)
	local ret = self:do_compile(env, rule, id)
	env:unregister(self)
	compound:add(ret)
	compound:add(grammar_dg.CompoundFinish:new(env:gid(), rule, id))
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

	ret = grammar_dg.Retain:new(env:gid(), haka.packet_mode() == 'passthrough')

	iter = grammar_dg.RecordStart:new(env:gid(), rule, id, self.named, self.resultclass)
	self:property_setup(iter)
	ret:add(iter)

	for i, entity in ipairs(self.entities) do
		local next = entity:compile(env, self.rule or rule, i)
		if iter then
			iter:add(next)
			iter = next
		else
			ret = next
			iter = ret
		end
	end

	local pop = grammar_dg.RecordFinish:new(env:gid(), self.named ~= nil)
	self:post_setup(pop)

	for name, f in pairs(self.extra_entities) do
		pop:extra(name, f)
	end
	iter:add(pop)
	iter:add(grammar_dg.Release:new(env:gid()))

	return ret
end


grammar_int.Sequence = class.class('Sequence', grammar_int.Compound)

function grammar_int.Sequence.method:__init(entities)
	self.entities = entities
end

function grammar_int.Sequence.method:do_compile(env, rule, id)
	local iter, ret

	ret = grammar_dg.RecordStart:new(env:gid(), rule, id, self.named, self.resultclass)
	self:property_setup(ret)
	iter = ret

	for i, entity in ipairs(self.entities) do
		if entity.named then
			log.warning("named element '%s' are not supported in sequence", entity.named)
		end

		local next = entity:compile(env, self.rule or rule, i)
		if iter then
			iter:add(next)
			iter = next
		else
			ret = next
			iter = ret
		end
	end

	local pop = grammar_dg.RecordFinish:new(env:gid(), self.named ~= nil)
	self:post_setup(pop)
	iter:add(pop)

	return ret
end


grammar_int.Union = class.class('Union', grammar_int.Compound)

function grammar_int.Union.method:__init(entities)
	self.entities = entities
end

function grammar_int.Union.method:do_compile(env, rule, id)
	local ret = grammar_dg.UnionStart:new(env:gid(), rule, id, self.named, self.resultclass)

	for i, entity in ipairs(self.entities) do
		local next = entity:compile(env, self.rule or rule, i)
		ret:add(next)
		ret:add(grammar_dg.UnionRestart:new(env:gid()))
	end

	ret:add(grammar_dg.UnionFinish:new(env:gid(), self.named))
	return ret
end

grammar_int.Try = class.class('Try', grammar_int.Compound)

function grammar_int.Try.method:__init(cases)
	self.cases = cases
end

function grammar_int.Try.method:do_compile(env, rule, id)
	local first_try = nil
	local previous_try = nil
	local finish = grammar_dg.TryFinish:new(env:gid(), self.rule or rule, id, self.named)

	-- For each case we prepend a catch entity
	-- and we chain catch entities together to try every cases
	for i, entity in ipairs(self.cases) do
		local next = entity:compile(env, self.rule or rule, i)

		local try = grammar_dg.TryStart:new(env:gid(), self.rule or rule, id, self.named, self.resultclass)
		try:add(next)
		try:add(finish)
		if previous_try then
			previous_try:catch(try)
		end
		previous_try = try
		first_try = first_try or try
	end

	if not first_try then
		error("cannot create a try element without any case")
	end

	-- End with error if everything failed
	previous_try:catch(grammar_dg.Error:new(env:gid(), self.rule or rule, "cannot find a successful try case"))

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
	local ret = grammar_dg.Branch:new(env:gid(), rule, id, self.selector)

	for key, entity in pairs(self.cases) do
		if key ~= 'default' then
			local next = entity:compile(env, self.rule or rule, key)
			ret:case(key, next)
		end
	end

	local default = self.cases.default
	if default == 'error' then
		ret._next = grammar_dg.Error:new(env:gid(), self.rule or rule, "invalid case")
	elseif default ~= 'continue' then
		local next = default:compile(env, self.rule or rule, 'default')
		ret._next = next
	end

	return ret
end


grammar_int.Array = class.class('Array', grammar_int.Compound)

function grammar_int.Array.method:__init(entity)
	self.entity = entity
end


function grammar_int.Array.method:do_compile(env, rule, id)
	local start = grammar_dg.ArrayStart:new(env:gid(), rule, id, self.named, self.create, self.resultclass)
	self:property_setup(start)

	local loop = grammar_dg.Branch:new(env:gid(), nil, nil, self.more)
	local push = grammar_dg.ArrayPush:new(env:gid(), rule, id)
	local inner = self.entity:compile(env, self.rule or rule, id)
	local pop = grammar_dg.ArrayPop:new(env:gid())
	local finish = grammar_dg.ResultPop:new(env:gid())

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

grammar_int.Number = class.class('Number', grammar_int.Entity)

function grammar_int.Number.method:__init(bits, endian)
	self.bits = bits
	self.endian = endian or 'big'
end

function grammar_int.Number.method:do_compile(env, rule, id)
	local ret = grammar_dg.Number:new(env:gid(), rule, id, self.bits, self.endian, self.named)
	self:compile_setup(ret)
	return ret
end


grammar_int.Bytes = class.class('Bytes', grammar_int.Entity)

function grammar_int.Bytes.method:do_compile(env, rule, id)
	if self._untiltoken and not self._untilre then
		self._untilre = rem.re:compile("(?:"..self._untiltoken..")")
	end
	local ret = grammar_dg.Bytes:new(env:gid(), rule, id, self._count, self._untilre, self.named, self._chunked)
	self:compile_setup(ret)
	return ret
end

function grammar_int.Bytes.method:chunked(callback)
	local clone = self:clone()
	clone._chunked = callback
	return clone
end

function grammar_int.Bytes.method:count(count)
	local clone = self:clone()
	if clone._untiltoken ~= nil then
		error("bytes can't have both count and untiltoken")
	end

	if type(count) ~= 'function' then
		clone._count = function (self) return count end
	else
		clone._count = count
	end

	return clone
end

function grammar_int.Bytes.method:untiltoken(token)
	local clone = self:clone()
	if clone._count ~= nil then
		error("bytes can't have both count and untiltoken")
	end

	clone._untiltoken = token

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

	return grammar_dg.Bits:new(env:gid(), rule, id, self.bits)
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
	local ret = grammar_dg.Token:new(env:gid(), rule, id, self.pattern, self.re, self.named, self.raw)
	self:compile_setup(ret)
	return ret
end

grammar_int.Execute = class.class('Execute', grammar_int.Entity)

function grammar_int.Execute.method:__init(func)
	self.func = func
end

function grammar_int.Execute.method:do_compile(env, rule, id)
	return grammar_dg.Execute:new(env:gid(), rule, id, self.func)
end

grammar_int.Retain = class.class('Retain', grammar_int.Entity)

function grammar_int.Retain.method:__init(readonly)
	self.readonly = readonly
end

function grammar_int.Retain.method:do_compile(env, rule, id)
	return grammar_dg.Retain:new(env:gid(), self.readonly)
end

grammar_int.Release = class.class('Release', grammar_int.Entity)

function grammar_int.Release.method:do_compile(env, rule, id)
	return grammar_dg.Release:new(env:gid())
end

grammar_int.Empty = class.class('Empty', grammar_int.Entity)

function grammar_int.Empty.method:do_compile(env, rule, id)
	return grammar_dg.Empty:new(env:gid(), rule, id)
end

grammar_int.Error = class.class('Error', grammar_int.Entity)

function grammar_int.Error.method:__init(msg)
	self.msg = msg
end

function grammar_int.Error.method:do_compile(env, rule, id)
	return grammar_dg.Error:new(env:gid(), id, self.msg)
end

local GrammarProxy = class.class("GrammarProxy", grammar_int.Entity)

function GrammarProxy.method:__init(target)
	self._target = target
end

function GrammarProxy.method:do_compile(env, rule, id)
	local entity = env._grammar._rules[self._target]
	if not entity or entity._target == self._target then
		error(string.format("undefined entity: %s", self._target))
	end

	local clone = entity:clone()
	if self.named then clone.named = self.named end
	if self._apply then
		if not clone._apply then clone._apply = {} end
		table.merge(clone._apply, self._apply)
	end
	if self._converter then clone._converter = self._converter end
	if self._memoize  then clone._memoize = self._memoize end
	local ret = clone:compile(env, rule, id)
	return ret
end


--
-- Grammar description public API
--

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
	local args = table.pack(...)
	for i=1,args.n do
		local proxy = args[i]
		if not class.isa(proxy, GrammarProxy) then
			error("can only export named rules")
		end
		self._exports[proxy._target] = true
	end
end

function Grammar.method:extend(...)
	local args = table.pack(...)
	for i=1,args.n do
		local grammar = args[i]
		table.merge(self._rules, grammar._rules)
		-- We just need key of exports to recompile it
		table.merge(self._exports, grammar._exports)
	end
end

function Grammar.method:define(...)
	local args = table.pack(...)
	for i=1,args.n do
		local name = args[i]
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

local function new_c_grammar(name, def)
	assert(type(def) == 'function', "grammar definition must be a function")

	local g = Grammar:new(name)

	-- Add a metatable to the environment only during the definition
	-- of the grammar.
	local env = debug.getfenv(def)
	setmetatable(env, grammar_env(g))

	def()
	setmetatable(env, nil)

	local tmpl = ccomp:new(name, haka.config.ccomp.debug)

	-- Compile exported entities
	local no_export = true

	for name, _  in pairs(g._exports) do
		local value = g._rules[name]
		if not value then
			error("exported rule must be registered in the grammar: "..name)
		end

		local genv = GrammarEnv:new(g)
		local dgraph = value:compile(genv)
		g._exports[name] = dgraph

		tmpl:add_parser(name, dgraph)

		no_export = false
	end

	if no_export then
		log.warning("grammar '%s' does not have any exported element", g._name)
	end

	if haka.config.ccomp.graph then
		log.warning("dumping '%s' grammar graph to %s-grammar.dot", g._name, g._name)
		local f = io.open(string.format("%s-grammar.dot", g._name), "w+")
		g:dump_graph(f)
		f:close()
	end

	local cparser = require(tmpl:compile())

	for _, parser in pairs(tmpl._ctx._parsers) do
		local gdentity = g._exports[parser.name]

		g._exports[parser.name] = {
			parse = function(self, input, context)
				local ctx = cparser.ctx:new(input)
				ctx.user = context
				ctx:init(g._rules[parser.name], true) -- ctx init require head of DG for now
				local ret = cparser.grammar[parser.fname](ctx._ctx)
				while ret ~= 0 do
					tmpl._store[ret](ctx)
					ret = cparser.grammar[parser.fname](ctx._ctx)
				end
				return ctx._results[1], ctx:get_error()
			end,
			create = function(self, input, init)
				-- Fallback to lua parser for now
				return gdentity:create(input, init)
			end
		}
	end

	return g
end

local function new_lua_grammar(name, def)
	assert(type(def) == 'function', "grammar definition must be a function")

	local g = Grammar:new(name)

	-- Add a metatable to the environment only during the definition
	-- of the grammar.
	local env = debug.getfenv(def)
	setmetatable(env, grammar_env(g))

	def()
	setmetatable(env, nil)

	local file_name = name.."_grammar"
	local f

	-- Compile exported entities
	local no_export = true

	for name, _  in pairs(g._exports) do
		local value = g._rules[name]
		if not value then
			error("exported rule must be registered in the grammar: "..name)
		end

		local genv = GrammarEnv:new(g)
		g._exports[name] = value:compile(genv)

		no_export = false
	end

	if no_export then
		log.warning("grammar '%s' does not have any exported element", g._name)
	end

	if grammar.graph then
		log.warning("dumping '%s' grammar graph to %s-grammar.dot", g._name, g._name)
		local f = io.open(string.format("%s-grammar.dot", g._name), "w+")
		g:dump_graph(f)
		f:close()
	end

	return g
end

function grammar.new(name, def, luacomp)
	if not luacomp then
		return new_c_grammar(name, def)
	else
		return new_lua_grammar(name, def)
	end

end

haka.grammar = grammar


--
-- Grammar graph
--

local grammar_dg = {}

grammar_dg.Context = class('DGContext')

grammar_dg.Entity = class('DGEntity')

function grammar_dg.Entity.method:next(ctxs, input)
	self:apply(ctxs, input)
	return self._next
end

function grammar_dg.Entity.method:add(next)
	self._next = next
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
	while iter do
		iter = iter:next(ctxs, input)
	end
	return ctxs[1]
end

grammar_dg.Control = class('DGControl', grammar_dg.Entity)

grammar_dg.ContextPush = class('DGContextPush', grammar_dg.Entity)

function grammar_dg.ContextPush.method:__init(name)
	super(self).__init(self)
	self.name = name
end

function grammar_dg.ContextPush.method:apply(ctxs)
	local new = grammar_dg.Context:new()
	ctxs[#ctxs][self.name] = new
	table.insert(ctxs, new)
end

grammar_dg.ContextPop = class('DGContextPop', grammar_dg.Control)

function grammar_dg.ContextPop.method:apply(ctxs)
	ctxs[#ctxs] = nil
end

grammar_dg.Error = class('DGError', grammar_dg.Control)

function grammar_dg.Error.method:__init(msg)
	self.msg = msg
end

function grammar_dg.Error.method:apply()
	error(self.msg)
end

grammar_dg.Branch = class('DGBranch', grammar_dg.Control)

function grammar_dg.Branch.method:__init(selector)
	self.selector = selector
	self.cases = {}
end

function grammar_dg.Branch.method:case(key, entity)
	self.cases[key] = entity
end

function grammar_dg.Branch.method:next(ctxs, input)
	local next = self.cases[self.selector(ctxs[1])]
	if next then
		return next
	else
		return self._next
	end
end

function grammar_dg.Branch.method:lasts(lasts)
	super(self).lasts(self, lasts)

	for _, entity in pairs(self.cases) do
		entity:lasts(lasts)
	end

	return lasts
end


grammar_dg.Primitive = class('DGPrimitive', grammar_dg.Entity)

function grammar_dg.Primitive.method:apply(ctxs, input)
	self:parse(ctxs[#ctxs], input)
end

grammar_dg.Integer = class('DGInteger', grammar_dg.Primitive)

function grammar_dg.Integer.method:__init(size, endian, name)
	super(self).__init(self)
	self.size = size
	self.endian = endian
	self.name = name
end

function grammar_dg.Integer.method:parse(ctx, input)
	local sub = input:sub(self.size)

	if self.name then
		ctx:addproperty(self.name,
			function (ctx)
				return sub:asnumber(self.endian)
			end,
			function (ctx, newvalue)
				return sub:setnumber(newvalue, self.endian)
			end
		)
	end
end

grammar_dg.Bytes = class('DGBytes', grammar_dg.Primitive)

function grammar_dg.Bytes.method:__init(size, name)
	super(self).__init(self)
	self.size = size
	self.name = name
end

function grammar_dg.Bytes.method:parse(ctx, input)
	local sub = input:sub(self.size or -1)

	if self.name then
		ctx[self.name] = sub
	end
end


--
-- Grammar description
--

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
	
	if self.named then
		ret = grammar_dg.ContextPush:new(self.named)
		iter = ret
	end

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

	if self.named then
		local pop = grammar_dg.ContextPop:new()
		for _, last in ipairs(iter:lasts({})) do
			last:add(pop)
		end
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


grammar.Integer = class('Integer', grammar.Entity)

function grammar.Integer.method:__init(bits)
	self.bits = bits
end

function grammar.Integer.method:compile()
	return grammar_dg.Integer:new(self.bits, self.endian, self.named)
end

grammar.Integer._options = {}
function grammar.Regex._options.endianness(self, endian) self.endian = endian end


grammar.Bytes = class('Bytes', grammar.Entity)

function grammar.Bytes.method:compile()
	return grammar_dg.Bytes:new(self.count, self.named)
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

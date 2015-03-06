-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local rem = require("regexp/pcre")
local parseResult = require("parse_result")
local parse = require("parse")

local dg = {}
local log = haka.log_section("grammar")

--
-- Grammar direct graph
--

dg.Entity = class.class('DGEntity')

function dg.Entity.method:__init(gid, rule, id)
	self.gid = gid
	self.rule = rule
	self.id = id
end

function dg.Entity.method:getnexts()
	return { self._next }
end

function dg.Entity.method:_capply(ccomp)
	ccomp:apply_node(self)
end

function dg.Entity.method:next(ctx)
	return self._next
end

function dg.Entity.method:_create(ctx)
	return self:_apply(ctx)
end

function dg.Entity.method:_apply(ctx)
	return nil
end

function dg.Entity.method:_nexts(list)
	if self._next then
		table.insert(list, self._next)
	end
end

function dg.Entity.method:_add(next, set)
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

function dg.Entity.method:add(next)
	self:_add(next, {})
end

function dg.Entity.method:convert(converter, memoize)
	self.converter = converter
	self.memoize = memoize or false
end

function dg.Entity.method:validate(validate)
	self._validate = validate
end

function dg.Entity.method:add_apply(apply)
	if not self._post_apply then
		self._post_apply = {}
	end

	table.insert(self._post_apply, apply)
end

function dg.Entity.method:do_apply(value, ctx)
	if self._post_apply then
		local res = ctx:result()

		if self.converter then
			value = self.converter.get(value)
		end

		for _, apply in ipairs(self._post_apply) do
			apply(value, res, ctx)

			if ctx._error then break end
		end
	end
end

function dg.Entity.method:genproperty(obj, name, get, set)
	local fget, fset = get, set

	if self.converter then
		fget = function (this)
			return self.converter.get(get(this))
		end
		fset = function (this, newval)
			return set(this, self.converter.set(newval))
		end
	end

	if self.memoize then
		local memname = '_' .. name .. '_memoize'
		local oldget, oldset = fget, fset

		fget = function (this)
			local ret = rawget(this, memname)
			if not ret then
				ret = oldget(this)
				rawset(this, memname, ret)
			end
			return ret
		end
		fset = function (this, newval)
			rawset(this, memname, nil)
			return oldset(this, newval)
		end
	end

	if self._validate then
		local oldget, oldset = fget, fset
		local validate = self._validate

		fget = function (this)
			if this._validate[validate] then
				validate(this)
			end
			return oldget(this)
		end
		fset = function (this, newval)
			if newval == nil then
				this._validate[validate] = this
			else
				this._validate[validate] = nil
				return oldset(this, newval)
			end
		end
	end

	obj:addproperty(name, fget, fset)
end

function dg.Entity.method:parse(input, context)
	local ctx = parse.Context:new(input)
	ctx.user = context
	return ctx:parse(self)
end

function dg.Entity.method:create(input, init)
	local ctx = parse.Context:new(input, init)
	return ctx:create(self)
end

function dg.Entity.method:_dump_graph_descr()
end

function dg.Entity.method:_dump_graph_options()
	return ""
end

function dg.Entity.method:_dump_graph_node(file, ref)
	local label, extlabel

	label = string.format("%d: ", self.gid)
	if self.name then
		label = string.format("%s%s: %s", label, class.classof(self).name, self.name)
	else
		label = label..class.classof(self).name
	end

	extlabel = self:_dump_graph_descr()
	if extlabel then
		label = string.format("%s\\n%s", label, extlabel)
	end

	file:write(string.format('%s [label="%s"%s];\n', ref[self], label, self:_dump_graph_options()))
end

function dg.Entity.method:_dump_graph_edges(file, ref)
	if self._next then
		self._next:_dump_graph(file, ref)
		file:write(string.format('%s -> %s;\n', ref[self], ref[self._next]))
	end
end

function dg.Entity.method:_dump_graph(file, ref)
	if not ref[self] then
		ref[self] = string.format("N_%d", ref._index)
		ref._index = ref._index+1
		self:_dump_graph_node(file, ref)
		return self:_dump_graph_edges(file, ref)
	end
end

function dg.Entity.method:dump_graph(file)
	file:write("digraph grammar {\n")

	local ref = {}
	ref._index = 1
	self:_dump_graph(file, ref)

	file:write("}\n")
end

function dg.Entity.method:_ctrace()
	local name = class.classof(self).trace_name
	if not name or not self.id then
		-- skip trace
		return
	end

	local descr = self:_dump_graph_descr()
	if descr then
		return string.format("parsing %s %s", name, descr)
	else
		return string.format("parsing %s", name)
	end

end

function dg.Entity.method:trace(position, msg, ...)
	if self.id then
		local id = self.id
		if self.name then
			id = string.format("'%s'", self.name)
		end

		log.debug("in rule '%s' field %s gid %d: %s\n\tat byte %d: %s...",
			self.rule or "<unknown>", id or "<unknown>", self.gid,
			string.format(msg, ...), position.meter,
			safe_string(position:copy():sub(20):asstring()))
	end
end

function dg.Entity.method:_trace(position)
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

dg.Control = class.class('DGControl', dg.Entity)

function dg.Control.method:_dump_graph_options()
	return ',fillcolor="#dddddd",style="filled"'
end

dg.CompoundStart = class.class('DGCompoundStart', dg.Control)

function dg.CompoundStart.method:__init(gid, rule, id, resultclass)
	class.super(dg.CompoundStart).__init(self, gid, rule, id)
	self.resultclass = resultclass or parseResult.Result
end

function dg.CompoundStart.method:_capply(ccomp)
	ccomp:write[[
			ctx->compound_level++;
]]
end

function dg.CompoundStart.method:_apply(ctx)
	ctx:pushlevel()
end

dg.CompoundFinish = class.class('DGCompoundFinish', dg.Control)

function dg.CompoundFinish.method:_capply(ccomp)
	ccomp:write[[
			ctx->compound_level--;
			if (ctx->recurs_finish_level == ctx->compound_level && ctx->recurs_count > 0) {
				/* pop recursion */
				ctx->recurs_count--;
				ctx->next = ctx->recurs[ctx->recurs_count].node;
				ctx->recurs_finish_level = ctx->recurs[ctx->recurs_count].level;
				break;
			}
]]
end

function dg.CompoundFinish.method:_apply(ctx)
	ctx:poplevel()
end

dg.Recurs = class.class('DGRecurs', dg.Control)

dg.Recurs.trace_name = 'recursion'

function dg.Recurs.method:__init(gid, rule, id, recurs)
	class.super(dg.Recurs).__init(self, gid, rule, id)
	self._recurs = recurs
end

function dg.Recurs.method:_dump_graph_edges(file, ref)
	self._next:_dump_graph(file, ref)
	file:write(string.format('%s -> %s [label="recursion"];\n', ref[self], ref[self._recurs]))
	if self._next then
		self._next:_dump_graph(file, ref)
		file:write(string.format('%s -> %s [label="finish"];\n', ref[self], ref[self._next]))
	else
		file:write(string.format('%s -> end [label="finish"];\n', ref[self]))
		file:write('end [fillcolor="#ff0000",style="filled"];\n')
	end
end

function dg.Recurs.method:getnexts()
	return { self._recurs, self._next }
end

function dg.Recurs.method:next()
	return self._recurs
end

function dg.Recurs.method:_apply(ctx)
	ctx:pushrecurs(self._next)
end

dg.ResultPop = class.class('DGResultPop', dg.Control)

function dg.ResultPop.method:__init(gid)
	class.super(dg.ResultPop).__init(self, gid)
end

function dg.ResultPop.method:_apply(ctx)
	ctx:pop()
end

dg.RecordStart = class.class('DGRecordStart', dg.CompoundStart)

dg.RecordStart.trace_name = 'record'

function dg.RecordStart.method:__init(gid, rule, id, name, resultclass)
	class.super(dg.RecordStart).__init(self, gid, rule, id, resultclass)
	self.name = name
end

function dg.RecordStart.method:_capply(ccomp)
	class.super(dg.RecordStart)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.RecordStart.method:_apply(ctx)
	if self.name then
		local res = ctx:result()
		local new = ctx:push(self.resultclass:new(), self.name)

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

dg.RecordFinish = class.class('DGRecordFinish', dg.CompoundFinish)

function dg.RecordFinish.method:__init(gid, pop)
	class.super(dg.RecordFinish).__init(self, gid)
	self._extra = {}
	self._pop = pop
end

function dg.RecordFinish.method:_capply(ccomp)
	class.super(dg.RecordFinish)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.RecordFinish.method:extra(name, f)
	self._extra[name] = f
end

function dg.RecordFinish.method:_apply(ctx)
	local top = ctx:result(1)
	local res = ctx:result()
	if self._pop then
		ctx:pop()
	end

	for name, func in pairs(self._extra) do
		res:addproperty(name, func(res), nil)
	end

	self:do_apply(res, ctx)
end

dg.UnionStart = class.class('DGUnionStart', dg.CompoundStart)

dg.UnionStart.trace_name = 'union'

function dg.UnionStart.method:__init(gid, rule, id, name, resultclass)
	class.super(dg.UnionStart).__init(self, gid, rule, id, resultclass)
	self.name = name
end

function dg.UnionStart.method:_capply(ccomp)
	class.super(dg.UnionStart)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.UnionStart.method:_apply(ctx)
	if self.name then
		local res = ctx:result()
		local new = ctx:push(self.resultclass:new(), self.name)
	end

	ctx:mark(false)
	ctx:pushmark()
end

dg.UnionRestart = class.class('DGUnionRestart', dg.Control)

function dg.UnionRestart.method:_capply(ccomp)
	ccomp:write[[
			parse_ctx_seekmark(ctx);
]]
end

function dg.UnionRestart.method:_apply(ctx)
	ctx:seekmark()
end

dg.UnionFinish = class.class('DGUnionFinish', dg.CompoundFinish)

function dg.UnionFinish.method:__init(gid, pop)
	class.super(dg.UnionFinish).__init(self, gid)
	self._pop = pop
end

function dg.UnionFinish.method:_capply(ccomp)
	class.super(dg.UnionFinish)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.UnionFinish.method:_apply(ctx)
	local res = ctx:result()

	if self._pop then
		ctx:pop()
	end

	self:do_apply(res, ctx)

	ctx:popmark()
	ctx:unmark()
end

dg.TryStart = class.class('DGTryStart', dg.CompoundStart)

function dg.TryStart.method:__init(gid, rule, id, name, resultclass)
	class.super(dg.TryStart).__init(self, gid, rule, id, resultclass)
	self.name = name
	self._catch = nil
end

function dg.TryStart.method:_capply(ccomp)
	class.super(dg.TryStart)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.TryStart.method:catch(catch)
	self._catch = catch
end

function dg.TryStart.method:_dump_graph_edges(file, ref)
	self._next:_dump_graph(file, ref)
	file:write(string.format('%s -> %s;\n', ref[self], ref[self._next]))
	self._catch:_dump_graph(file, ref)
	file:write(string.format('%s -> %s [label="catch"];\n', ref[self], ref[self._catch]))
end

function dg.TryStart.method:_apply(ctx)
	-- Create a result ctx even if self.name is null
	-- in order to be able to delete it if case fail
	ctx:push(self.resultclass:new(), self.name)
	ctx:pushcatch(self._catch)
end

dg.TryFinish = class.class('DGTryFinish', dg.CompoundFinish)

function dg.TryFinish.method:__init(gid, rule, id, name)
	class.super(dg.TryFinish).__init(self, gid, rule, id)
	self.name = name
end

function dg.TryFinish.method:_capply(ccomp)
	class.super(dg.TryFinish)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.TryFinish.method:_apply(ctx)
	local res = ctx:result()
	ctx:pop()

	self:do_apply(res, ctx)

	if not self.name then
		-- Merge unnamed temp result ctx with parent result context
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
	end

	ctx:popcatch()
end

dg.ArrayStart = class.class('DGArrayStart', dg.CompoundStart)

dg.ArrayStart.trace_name = 'array'

function dg.ArrayStart.method:__init(gid, rule, id, name, create, resultclass)
	class.super(dg.ArrayStart).__init(self, gid, rule, id, resultclass or parseResult.ArrayResult)
	self.name = name
	self.create = create
end

function dg.ArrayStart.method:_capply(ccomp)
	class.super(dg.ArrayStart)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.ArrayStart.method:set_entity(entity)
	self.entity = entity
end

function dg.ArrayStart.method:_apply(ctx)
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

dg.ArrayFinish = class.class('DGArrayFinish', dg.CompoundFinish)

function dg.ArrayFinish.method:__init(gid)
	class.super(dg.ArrayFinish).__init(self, gid)
end

function dg.ArrayFinish.method:_capply(ccomp)
	class.super(dg.ArrayFinish)._capply(self, ccomp)
	ccomp:apply_node(self)
end

function dg.ArrayFinish.method:apply(ctx)
	local res = ctx:result()
	ctx:pop()

	self:do_apply(res, ctx)
end

dg.ArrayPush = class.class('DGArrayPush', dg.Control)

function dg.ArrayPush.method:__init(gid, rule, id)
	class.super(dg.ArrayPush).__init(self, gid, rule, id)
end

function dg.ArrayPush.method:_apply(ctx)
	self:trace(ctx.iter, "append array element")

	local res = ctx:result()
	if class.isa(res, parseResult.ArrayResult) then
		rawset(res, '_entitybegin', ctx.iter:copy())
	end
	local new = ctx:push(nil, #res+1)
	table.insert(res, new)
end

dg.ArrayPop = class.class('DGArrayPop', dg.Control)

function dg.ArrayPop.method:__init(gid)
	class.super(dg.ArrayPop).__init(self, gid)
end

function dg.ArrayPop.method:_apply(ctx)
	local entityresult = ctx:result()
	ctx:pop()

	if #ctx._results == 0 then
		-- On creation we might not have results anymore
		-- just skip it
		return
	end

	local arrayresult = ctx:result()
	if class.isa(arrayresult, parseResult.ArrayResult) then
		rawset(entityresult, '_sub', haka.vbuffer_sub(arrayresult._entitybegin, ctx.iter))
		rawset(arrayresult, '_entitybegin', nil)
		ctx.iter:split()
	end
end

dg.Error = class.class('DGError', dg.Control)

function dg.Error.method:__init(gid, id, msg)
	class.super(dg.Error).__init(self, gid, nil, id)
	self.msg = msg
end

function dg.Error.method:_dump_graph_options()
	return ',fillcolor="#ee0000",style="filled"'
end

function dg.Error.method:_apply(ctx)
	return ctx:error(self.msg)
end

dg.Execute = class.class('DGExecute', dg.Control)

function dg.Execute.method:__init(gid, rule, id, callback)
	class.super(dg.Error).__init(self, gid, rule, id)
	self.callback = callback
end

function dg.Execute.method:_apply(ctx)
	self.callback(ctx:result(), ctx)
end

dg.Retain = class.class('DGRetain', dg.Control)

function dg.Retain.method:__init(gid, readonly)
	class.super(dg.Error).__init(self, gid)
	self.readonly = readonly
end

function dg.Retain.method:_apply(ctx)
	-- We try to mark the incoming data, so wait for them
	-- to arrive before marking the end of a previous chunk
	ctx.iter:wait()

	ctx:mark(self.readonly)
end

dg.Release = class.class('DGRelease', dg.Control)

function dg.Release.method:__init(gid, rule, id)
	class.super(dg.Release).__init(self, gid, rule, id)
end

function dg.Release.method:_capply(ccomp)
	ccomp:unmark()
end

function dg.Release.method:_apply(ctx)
	ctx:unmark()
end

dg.Branch = class.class('DGBranch', dg.Control)

function dg.Branch.method:__init(gid, rule, id, selector)
	class.super(dg.Branch).__init(self, gid, rule, id)
	self.selector = selector
	self.cases = {}
end

function dg.Branch.method:case(key, entity)
	self.cases[key] = entity
end

function dg.Branch.method:_dump_graph_edges(file, ref)
	for name, case in pairs(self.cases) do
		case:_dump_graph(file, ref)
		file:write(string.format('%s -> %s [label="%s"];\n', ref[self], ref[case], name))
	end

	if self._next then
		self._next:_dump_graph(file, ref)
		file:write(string.format('%s -> %s [label="default"];\n', ref[self], ref[self._next]))
	end
end

function dg.Branch.method:capply(ccomp, parser)
	local cases = {}
	local cases_map = {}
	for name, entity in pairs(self.cases) do
		cases[name] = entity
	end
	cases["default"] = self._next

	for name, case in pairs(cases) do
		local id = parser:register(case)
		cases_map[name] = id
	end

	return function(ctx)
		local branch = self.selector(ctx:result(), ctx)
		local case = cases_map[branch]
		if case then
			self:trace(ctx.iter, "select branch '%s'", case)
		else
			self:trace(ctx.iter, "select branch 'default'")
			case = cases_map["default"]
		end
		ctx._ctx.next = case
	end
end

function dg.Branch.method:getnexts()
	local cases = {}
	for _, entity in pairs(self.cases) do
		cases[#cases + 1] = entity
	end
	cases[#cases + 1] = self._next

	return cases
end

function dg.Branch.method:next(ctx)
	local case = self.selector(ctx:result(), ctx)

	local next = self.cases[case]

	if next then
		self:trace(ctx.iter, "select branch '%s'", case)
		return next
	else
		self:trace(ctx.iter, "select branch 'default'")
		return self._next
	end
end

function dg.Branch.method:_nexts(list)
	for _, value in pairs(self.cases) do
		table.insert(list, value)
	end

	return class.super(dg.Branch)._nexts(self, list)
end

dg.Primitive = class.class('DGPrimitive', dg.Entity)

function dg.Primitive.method:_apply(ctx)
	return self:_parse(ctx:result(), ctx.iter, ctx)
end

function dg.Primitive.method:_create(ctx)
	return self:_init(ctx:result(), ctx.iter, ctx, ctx.current_init)
end

dg.Number = class.class('DGNumber', dg.Primitive)

dg.Number.trace_name = 'number'

function dg.Number.method:__init(gid, rule, id, size, endian, name)
	class.super(dg.Number).__init(self, gid, rule, id)
	self.size = size
	self.endian = endian
	self.name = name
end

function dg.Number.method:_dump_graph_descr()
	return string.format("%d bits (%s endian)", self.size, self.endian or 'big')
end

function dg.Number.method:_capply(ccomp)
	ccomp:write([[
			const int size = (ctx->bitoffset + %d + 7) >> 3;
			const int bit = (ctx->bitoffset + %d) & 0x7;
			const bool iscontinue = vbuffer_iterator_isvalid(&ctx->reg0_iter);

			struct vbuffer_iterator *iter = iscontinue ? &ctx->reg0_iter : ctx->iter;

			if (!vbuffer_iterator_check_available(iter, size, NULL) ) {
				if (vbuffer_iterator_iseof(iter)) {
					error("Not enought data");
					ctx->next = FINISH;
					break;
				}

				/* Need to wait for more data */
				if (!iscontinue) {
					vbuffer_iterator_copy(iter, &ctx->reg0_iter);

					/*if (ctx->retains.count == 0)*/ {
						vbuffer_iterator_mark(&ctx->reg0_iter, true);
					}
				}

				/* Move the iterator to the end */
				vbuffer_iterator_advance(ctx->iter, ALL);

				/* We now need to wait for more data */
]], self.size, self.size)

	ccomp:call(ccomp.waitcall)

	ccomp:write([[
				break;
			}
			else {
				if (iscontinue) {
					vbuffer_iterator_unmark(iter);
				}

				vbuffer_sub_create_from_position(&ctx->reg0_sub, iter, size);
				vbuffer_iterator_advance(iter, size - (bit != 0 ? 1 : 0));

				if (iscontinue) {
					vbuffer_iterator_move(ctx->iter, iter);
					vbuffer_iterator_clear(&ctx->reg0_iter);
				}

				ctx->reg0_int = (ctx->bitoffset == 0 && bit == 0);
				ctx->reg1_int = ctx->bitoffset;
				ctx->bitoffset = bit;
]])

	if self._post_apply then
		ccomp:write([[
			if (ctx->reg0_int) {
				ctx->reg0_long = vbuffer_asnumber(ctx->reg0_sub, %d);
			}
			else {
				ctx->reg0_long = vbuffer_asbits(ctx->reg0_sub, ctx->reg1_int, %d, %d);
			}
]], self.endian == 'big', self.size, self.endian == 'big')
	end

	if self._post_apply or self.name then
		ccomp:call(ccomp:store(function (ctx)
			local sub = ctx._ctx.reg0_sub
			local aligned = ctx._ctx.reg0_int ~= 0
			local bitoffset = ctx._ctx.reg1_int
			local res = ctx:result()

			if self.name then
				if aligned then
					self:genproperty(res, self.name,
						function (this) return sub:asnumber(self.endian) end,
						function (this, newvalue) return sub:setnumber(newvalue, self.endian) end
					)
				else
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

			if self._post_apply then
				local value = ctx._ctx.reg0_long;
				self:do_apply(value, ctx)
			end
		end), "apply")
	end

	ccomp:write[[
		}
]]

end

function dg.Number.method:capply()
	return function (ctx)
		local sub = ctx._ctx.reg0_sub
		local aligned = ctx._ctx.reg0_int ~= 0
		local bitoffset = ctx._ctx.reg1_int
		local res = ctx:result()

		if self.name then
			if aligned then
				self:genproperty(res, self.name,
					function (this) return sub:asnumber(self.endian) end,
					function (this, newvalue) return sub:setnumber(newvalue, self.endian) end
				)
			else
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

		if self._post_apply then
			local value = ctx._ctx.reg0_long;
			self:do_apply(value, ctx)
		end
	end
end

function dg.Number.method:_parse(res, input, ctx)
	local bitoffset = ctx._bitoffset
	local size, bit = math.ceil((bitoffset + self.size) / 8), (bitoffset + self.size) % 8

	local sub = input:copy():sub(size)
	if bit ~= 0 then
		input:advance(size-1)
	else
		input:advance(size)
	end

	ctx._bitoffset = bit

	if self.name then
		if bitoffset == 0 and bit == 0 then
			self:genproperty(res, self.name,
				function (this) return sub:asnumber(self.endian) end,
				function (this, newvalue) return sub:setnumber(newvalue, self.endian) end
			)
		else
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

	if self._post_apply then
		local value
		if bitoffset == 0 and bit == 0 then
			value = sub:asnumber(self.endian)
		else
			value = sub:asbits(bitoffset, self.size, self.endian)
		end

		self:do_apply(value, ctx)
	end
end

function dg.Number.method:_init(res, input, ctx, init)
	self:_parse(res, input, ctx)

	if self.name and ctx.init then
		if init then
			local initval = init[self.name]
			if initval then
				res[self.name] = initval
			elseif self._validate then
				res._validate[self._validate] = res
			end
		else
			res[self.name] = nil
		end
	end
end

dg.Bits = class.class('DGBits', dg.Primitive)

function dg.Bits.method:__init(gid, rule, id, size)
	class.super(dg.Bits).__init(self, gid, rule, id)
	self.size = size
end

function dg.Bits.method:_parse(res, input, ctx)
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

function dg.Bits.method:_init(res, input, ctx, init)
end

dg.Bytes = class.class('DGBytes', dg.Primitive)

function dg.Bytes.method:__init(gid, rule, id, size, untilre, name, chunked_callback)
	class.super(dg.Bytes).__init(self, gid, rule, id)
	self.size = size
	self.name = name
	self.untilre = untilre
	self.chunked_callback = chunked_callback
end

function dg.Bytes.method:_parse(res, iter, ctx)
	if ctx._bitoffset ~= 0 then
		return ctx:error("byte primitive requires aligned bits")
	end

	local sub
	local size
	if self.size then
		size = self.size(res, ctx)
		if size < 0 then
			return ctx:error("byte count must not be negative, got %d", size)
		end
	else
		size = 'all'
	end

	local function create_property(sub)
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

	local function chunked_callback(sub, isend)
		if self.chunked_callback then
			self.chunked_callback(res, sub, isend, ctx)
		end
	end

	-- Easiest case
	if not self.chunked_callback and not self.untilre then
		create_property(iter:sub(size))
		return
	end

	-- Complexe case
	-- We have to go all over the bytes to
	--   - pass it to chunked_callback
	--   - check it against untilre

	-- called with size == 0 we just need to notify for stream end
	if self.chunked_callback and size == 0 then
		-- Call the callback to notify for the stream end
		self.chunked_callback(res, nil, true, ctx)
		return
	end

	local begin = iter:copy()
	local last_chunked
	local sink
	if self.untilre then
		sink = self.untilre:create_sink()
		begin:mark(haka.packet_mode() == 'passthrough')
	end

	while size == 'all' or size > 0 do
		iter:wait()
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

		if self.untilre then
			local match, mbegin, mend = sink:feed(sub, sub == nil)

			if mbegin then
				last_chunked = last_chunked or sub:pos('begin')
				chunked_callback(haka.vbuffer_sub(last_chunked, mbegin), match or iter.iseof)
				last_chunked = mbegin
			end

			-- No more partial match update chunked_callback
			if not match and not sink:ispartial() and last_chunked then
				chunked_callback(haka.vbuffer_sub(last_chunked, sub:pos('begin')), iter.iseof)
				last_chunked = nil
			end

			if not last_chunked and sub then
				-- no (partial) match ever
				-- pass full buffer to chunked_callback
				chunked_callback(sub, iter.iseof)
			end

			if match then
				iter:move_to(last_chunked)

				if not mbegin then
					chunked_callback(nil, true)
				end

				break
			end
		else
			chunked_callback(sub, size == 0 or iter.iseof)
		end

		if not sub then break end
	end

	if begin and self.untilre then
		begin:unmark()
	end

	if not self.chunked_callback then
		create_property(haka.vbuffer_sub(begin, iter))
	end
end

function dg.Bytes.method:_init(res, input, ctx, init)
	if self.name and init then
		local initval = init[self.name]
		if initval then
			sub:replace(initval)
		end
	end
end

dg.Token = class.class('DGToken', dg.Primitive)

dg.Token.trace_name = 'token'

function dg.Token.method:__init(gid, rule, id, pattern, re, name, raw)
	class.super(dg.Token).__init(self, gid, rule, id)
	self.pattern = pattern
	self.re = re
	self.name = name
	self.raw = raw
end

function dg.Token.method:_dump_graph_descr()
	return string.format("/%s/", safe_string(self.pattern))
end

function dg.Token.method:_parse(res, iter, ctx)
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

			if self.name or self._post_apply then
				local result = haka.vbuffer_sub(begin, mend)

				if self.raw then
					if self.name then
						self:genproperty(res, self.name,
							function (this) return result end)
					end

					self:do_apply(res, ctx)
				else
					local string = result:asstring()

					self:do_apply(string, ctx)

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
				end
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

	ctx:update(begin)
	return ctx:error("token /%s/ doesn't match", self.pattern)
end

dg.Empty = class.class('DGEmpty', dg.Control)

function dg.Empty.method:__init(gid, rule, id)
	class.super(dg.Empty).__init(self, gid, rule, id)
end

function dg.Token.method:_init(res, input, ctx, init)
	-- Should have been initialized
	return self:_parse(res, input, ctx)
end

return dg

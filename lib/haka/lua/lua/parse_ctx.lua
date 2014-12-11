-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local parseResult = require('parse_result')

local ffi = require('ffi')
local ffibinding = require('ffibinding')

ffi.cdef[[
	bool parse_ctx_new_ffi(struct parse_ctx_object *parse_ctx, void *iter);
	void parse_ctx_free(struct parse_ctx *ctx);
	struct lua_ref *parse_ctx_get_ref(void *ctx);

	struct parse_ctx {
		int run;
		int node;
	};
]]

ffibinding.create_type{
	cdef = "struct parse_ctx",
	prop = {
	},
	meth = {
	},
	destroy = ffi.C.parse_ctx_free,
	ref = ffi.C.parse_ctx_get_ref,
}

local parse_ctx_new = ffibinding.object_wrapper("struct parse_ctx", ffibinding.handle_error(ffi.C.parse_ctx_new_ffi), true)

--
-- Parse C Context
--

CContext = class.class('ParseCContext')

CContext.property.current_init = {
	get = function (self)
		if self._initresults then
			return self._initresults[#self._initresults]
		end
	end
}

CContext.property.init = {
	get = function (self) return self._initresults ~= nil end
}

CContext.property.retain_mark = {
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

function CContext.method:__init(iter, init)
	self._ctx = parse_ctx_new(iter)
	self.iter = iter
	self._bitoffset = 0
	self._marks = {}
	self._catches = {}
	self._results = {}
	self._validate = {}
	self._retain_mark = {}

	if init then
		self._initresults = { init }
	end

	self.iter.meter = 0
end

function CContext.method:result(idx)
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

function CContext.method:mark(readonly)
	local mark = self.iter:copy()
	mark:mark(readonly)
	table.insert(self._retain_mark, mark)
end

function CContext.method:unmark()
	local mark = self.retain_mark
	self._retain_mark[#self._retain_mark] = nil
	mark:unmark()
end

function CContext.method:update(iter)
	self.iter:move_to(iter)
end

function CContext.method:lookahead()
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

function CContext.method:init(entity, all)
	if entity.resultclass then
		self:push(entity.resultclass:new())
	else
		self:push(parseResult.Result:new())
	end
	self:result(1).validate = revalidate

	if all then self._level = 1
	else self._level = 0 end

	self._level_exit = 0
	self._error = nil
end

function CContext.method:pop()
	assert(#self._results > 0)

	self._results[#self._results] = nil

	if self._initresults then
		self._initresults[#self._initresults] = nil
	end
end

function CContext.method:push(result, name)
	local new = result or parseResult.Result:new()
	rawset(new, '_validate', self._validate)
	self._results[#self._results+1] = new
	if self._initresults then
		local curinit = self._initresults[#self._initresults]
		if curinit then
			self._initresults[#self._initresults+1] = curinit[name]
		else
			self._initresults[#self._initresults+1] = nil
		end
	end
	return new
end

function CContext.method:pushmark()
	self._marks[#self._marks+1] = {
		iter = self.iter:copy(),
		bitoffset = self._bitoffset,
		max_meter = 0,
		max_iter = self.iter:copy(),
		max_bitoffset = self._bitoffset
	}
end

function CContext.method:popmark(seek)
	local seek = seek or false
	local mark = self._marks[#self._marks]

	if seek then
		self.iter:move_to(mark.max_iter)
		self._bitoffset = mark.max_bitoffset
	end

	self._marks[#self._marks] = nil
end

function CContext.method:seekmark()
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

function CContext.method:pushcatch(entity)
	-- Should have created a ctx result
	self:mark(false)
	self:pushmark()

	self._catches[#self._catches+1] = {
		entity = entity,
		retain_count = #self._retain_mark,
		mark_count = #self._marks,
		result_count = #self._results,
	}
end

function CContext.method:catch()
	if #self._catches == 0 then
		return nil
	end

	local catch = self._catches[#self._catches]

	-- Unmark each retain started in failing case
	while #self._retain_mark > catch.retain_count do
		self:unmark()
	end

	-- remove all marks done in failing case
	while #self._marks > catch.mark_count do
		self:popmark(false)
	end

	-- remove all result ctx
	while #self._results > catch.result_count do
		self:pop()
	end

	-- Also remove result created by Try
	-- Should be done in Try entity but we can't
	self:pop()

	self:seekmark()
	self:popcatch()

	return catch.entity
end

function CContext.method:popcatch(entity)
	assert(#self._catches)
	local catch = self._catches[#self._catches]

	self:popmark(false)
	self:unmark()
	self._catches[#self._catches] = nil

	return catch
end

function CContext.method:error(description, ...)
	if self._error then
		error("multiple parse errors raised")
	end

	local context = {}
	description = string.format(description, ...)
	self._error = ParseError:new(self.iter, nil, description)
end

return CContext

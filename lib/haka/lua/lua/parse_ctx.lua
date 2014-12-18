-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <haka/config.h>


local class = require('class')
local parseResult = require('parse_result')

local parse_ctx_new

#ifdef HAKA_FFI

local ffi = require('ffi')
local ffibinding = require('ffibinding')

ffi.cdef[[
	bool parse_ctx_new_ffi(struct parse_ctx_object *parse_ctx, void *iter);
	void parse_ctx_free(struct parse_ctx *ctx);
	struct lua_ref *parse_ctx_get_ref(void *ctx);

	void parse_ctx_mark(struct parse_ctx *ctx, bool readonly);
	void parse_ctx_unmark(struct parse_ctx *ctx);
	void parse_ctx_pushmark(struct parse_ctx *ctx);
	void parse_ctx_popmark(struct parse_ctx *ctx, bool seek);
	void parse_ctx_seekmark(struct parse_ctx *ctx);
	void parse_ctx_update_error(struct parse_ctx *ctx, const char id[], const char rule[]);
	void parse_ctx_error(struct parse_ctx *ctx, const char desc[]);

	struct parse_ctx {
		int run;
		int node;
		int bitoffset;
	};
]]

ffibinding.create_type{
	cdef = "struct parse_ctx",
	prop = {
	},
	meth = {
		mark = ffi.C.parse_ctx_mark,
		unmark = ffi.C.parse_ctx_unmark,
		pushmark = ffi.C.parse_ctx_pushmark,
		popmark = ffi.C.parse_ctx_popmark,
		seekmark = ffi.C.parse_ctx_seekmark,
		update_error = ffi.C.parse_ctx_update_error,
		error = ffi.C.parse_ctx_error,
	},
	destroy = ffi.C.parse_ctx_free,
	ref = ffi.C.parse_ctx_get_ref,
}

parse_ctx_new = ffibinding.object_wrapper("struct parse_ctx", ffibinding.handle_error(ffi.C.parse_ctx_new_ffi), true)

#else

parse_ctx_new = haka.parse_ctx
haka.parse_ctx = nil

#endif

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

CContext.property._bitoffset = {
	get = function (self)
		return self._ctx.bitoffset
	end,
	set = function (self, val)
		self._ctx.bitoffset = val
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
	self._catches = {}
	self._results = {}
	self._validate = {}

	if init then
		self._initresults = { init }
	end

	self.iter.meter = 0
end

function CContext.method:mark(readonly)
	return self._ctx:mark(readonly)
end

function CContext.method:unmark()
	return self._ctx:unmark()
end

function CContext.method:pushmark()
	return self._ctx:pushmark()
end

function CContext.method:popmark(seek)
	local seek = seek or false
	return self._ctx:popmark(seek)
end

function CContext.method:seekmark()
	return self._ctx:seekmark()
end

function CContext.method:error(desc, ...)
	local desc = string.format(desc, ...)
	return self._ctx:error(desc)
end

function CContext.method:update_error(id, rule)
	local id = id or "<unknown>"
	local rule = rule or "<unknown>"
	return self._ctx:update_error(id, rule)
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

return CContext

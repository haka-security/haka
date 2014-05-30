-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local parseResult = require('parse_result')

local parse = {}

parse.max_recursion = 10

--
-- Parsing Error
--

local ParseError = class.class("ParseError")

function ParseError.method:__init(position, field, description, ...)
	self.position = position
	self.field = field
	self.description = description
end

function ParseError.method:context()
	local iter = self.position:copy()
	return string.format("parse error context: %s...", safe_string(iter:sub(100):asstring()))
end

function ParseError.method:__tostring()
	return string.format("parse error at byte %d for field %s in %s: %s",
		self.position.meter, self.field.id or "<unknown>", self.field.rule or "<unknown>",
		self.description)
end

--
-- Parse Context
--

parse.Context = class.class('ParseContext')

parse.Context.property.current_init = {
	get = function (self)
		if self._initresults then
			return self._initresults[#self._initresults]
		end
	end
}

parse.Context.property.init = {
	get = function (self) return self._initresults ~= nil end
}

parse.Context.property.retain_mark = {
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

function parse.Context.method:__init(iter, topresult, init)
	self.iter = iter
	self._bitoffset = 0
	self._marks = {}
	self._catches = {}
	self._results = {}
	self._validate = {}
	self._retain_mark = {}
	self._recurs = {}
	self._level = 0
	self:push(topresult)

	if init then
		self._initresults = { init }
	end

	self:result(1).validate = revalidate

	self.iter.meter = 0
end

function parse.Context.method:result(idx)
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

function parse.Context.method:mark(readonly)
	local mark = self.iter:copy()
	mark:mark(readonly)
	table.insert(self._retain_mark, mark)
end

function parse.Context.method:unmark()
	local mark = self.retain_mark
	self._retain_mark[#self._retain_mark] = nil
	mark:unmark()
end

function parse.Context.method:update(iter)
	self.iter:move_to(iter)
end

function parse.Context.method:lookahead()
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

function parse.Context.method:parse(entity)
	return self:_traverse(entity, "_apply", 1)
end

function parse.Context.method:create(entity)
	return self:_traverse(entity, "_create", 0)
end

function parse.Context.method:_traverse(entity, f, all)
	local iter = entity
	local err

	self._level = all
	self._level_exit = 0

	while iter do
		self._start_rec = false

		iter:_trace(self.iter)
		err = iter[f](iter, self)
		if err then
			iter = self:catch()
			if not iter then
				haka.log.debug("grammar", tostring(err))
				haka.log.debug("grammar", err:context())
				break
			else
				haka.log.debug("grammar", "catched: %s", err)
				haka.log.debug("grammar", "         %s", err:context())
				err = nil
			end
		else
			iter = iter:next(self)
		end

		while not self._start_rec and self._level == self._level_exit do
			iter = self:poprecurs()
			if not iter then break end
		end
	end
	return self._results[1], err
end

function parse.Context.method:pushlevel()
	self._level = self._level+1
end

function parse.Context.method:poplevel()
	self._level = self._level-1
end

function parse.Context.method:pop()
	assert(#self._results > 0)

	self._results[#self._results] = nil

	if self._initresults then
		self._initresults[#self._initresults] = nil
	end
end

function parse.Context.method:push(result, name)
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

function parse.Context.method:pushmark()
	self._marks[#self._marks+1] = {
		iter = self.iter:copy(),
		bitoffset = self._bitoffset,
		max_meter = 0,
		max_iter = self.iter:copy(),
		max_bitoffset = self._bitoffset
	}
end

function parse.Context.method:popmark(seek)
	local seek = seek or false
	local mark = self._marks[#self._marks]

	if seek then
		self.iter:move_to(mark.max_iter)
		self._bitoffset = mark.max_bitoffset
	end

	self._marks[#self._marks] = nil
end

function parse.Context.method:seekmark()
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

function parse.Context.method:pushcatch(entity)
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

function parse.Context.method:catch()
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

function parse.Context.method:popcatch(entity)
	assert(#self._catches)
	local catch = self._catches[#self._catches]

	self:popmark(false)
	self:unmark()
	self._catches[#self._catches] = nil

	return catch
end

function parse.Context.method:pushrecurs(finish)
	if #self._recurs >= parse.max_recursion then
		error("grammar max recursion reached")
	end

	self._recurs[#self._recurs+1] = {
		elem = finish,
		level = self._level_exit
	}

	self._level_exit = self._level
	self._start_rec = true
end

function parse.Context.method:poprecurs()
	-- Don't throw error if no more recurs as it can happen on normal end
	local state = self._recurs[#self._recurs]
	if state then
		self._recurs[#self._recurs] = nil
		self._level_exit = state.level

		return state.elem
	else
		self._level = 0
		return nil
	end
end

function parse.Context.method:error(position, field, description, ...)
	local context = {}
	description = string.format(description, ...)

	position = position or self.iter

	return ParseError:new(position, field, description)
end

return parse

-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')
local parseResult = require('parse_result')

local parse = {}
local log = haka.log_section("grammar")

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

function parse.Context.method:__init(iter, init)
	self.iter = iter
	self._bitoffset = 0
	self._marks = {}
	self._catches = {}
	self._results = {}
	self._validate = {}
	self._retain_mark = {}
	self._recurs = {}
	self._level = 0

	if init then
		self._initresults = { init }
	end

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
	return self:_traverse(entity, "_apply", true)
end

function parse.Context.method:create(entity)
	return self:_traverse(entity, "_create", false)
end

function parse.Context.method:_traverse(entity, f, all)
	local iter = entity
	local err

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

	while iter do
		-- This flag is updated when starting a recursion.
		self._start_rec = false

		iter:_trace(self.iter)
		iter[f](iter, self)

		if self._error then
			self._error.field = iter

			iter = self:catch()
			if not iter then
				log.debug("%s", tostring(self._error))
				log.debug("%s", self._error:context())
				break
			else
				log.debug("catched: %s", self._error)
				log.debug("         %s", self._error:context())
				self._error = nil
			end
		else
			iter = iter:next(self)
		end

		-- When the current level reach _level_exit, we should pop a recursion
		-- level or exit if none. If the recursion was just started, the check
		-- self._level == self._level_exit is always true. We need then to handle
		-- it correctly using the flag self._start_rec.
		while not self._start_rec and self._level == self._level_exit do
			iter = self:poprecurs()
			if not iter then break end
		end
	end
	return self._results[1], self._error
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

function parse.Context.method:error(description, ...)
	if self._error then
		error("multiple parse errors raised")
	end

	local context = {}
	description = string.format(description, ...)
	self._error = ParseError:new(self.iter, nil, description)
end

return parse

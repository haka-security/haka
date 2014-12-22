-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

--
-- Parsing Error
--

local ParseError = class.class("ParseError")

function ParseError.method:__init(position, id, rule, description, ...)
	self.position = position
	self.id = id
	self.rule = rule
	self.description = description
end

function ParseError.method:context()
	local iter = self.position:copy()
	return string.format("parse error context: %s...", safe_string(iter:sub(100):asstring()))
end

function ParseError.method:__tostring()
	return string.format("parse error at byte %d for field %s in %s: %s",
		self.position.meter, self.id or "<unknown>", self.rule or "<unknown>",
		self.description)
end


return {
    ParseError = ParseError
}

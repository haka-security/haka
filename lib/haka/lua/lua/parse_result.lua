-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local class = require('class')

local result = {}

result.Result = class.class('Result')

result.ArrayResult = class.class('ArrayResult', result.Result)

function result.ArrayResult.method:_init(iter, entity, create)
	rawset(self, '_begin', iter)
	rawset(self, '_entity', entity)
	rawset(self, '_create', create)
end

function result.ArrayResult.method:remove(el)
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

function result.ArrayResult.method:append(init)
	if not self._create then
		error("missing create function in array options")
	end
	local vbuf, result = self._create(self._entity, init)
	if #self > 0 then
		local sub = rawget(self, #self)._sub
		rawset(result, '_sub', sub:pos('end'):insert(vbuf))
		table.insert(self, result)
	else
		rawget(self, '_begin'):insert(vbuf)
	end
end

return result

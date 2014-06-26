-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

local color = {}

if hakainit.stdout_support_colors() then
	color.clear = string.char(0x1b) .. "[0m"
	color.bold = string.char(0x1b) .. "[1m"
	color.black = string.char(0x1b) .. "[30m"
	color.red = string.char(0x1b) .. "[31m"
	color.green = string.char(0x1b) .. "[32m"
	color.yellow = string.char(0x1b) .. "[33m"
	color.blue = string.char(0x1b) .. "[34m"
	color.magenta = string.char(0x1b) .. "[35m"
	color.cyan = string.char(0x1b) .. "[36m"
	color.white = string.char(0x1b) .. "[37m"
else
	color.clear = ""
	color.bold = ""
	color.black = ""
	color.red = ""
	color.green = ""
	color.yellow = ""
	color.blue = ""
	color.magenta = ""
	color.cyan = ""
	color.white = ""
end

return color

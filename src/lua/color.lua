
local color = {}

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

return color

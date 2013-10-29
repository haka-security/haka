
require('stats')
local color = require('color')

haka.on_exit(function()
	haka.debug.interactive.enter(color.green .. color.bold .. ">  " .. color.clear, color.green .. color.bold .. ">> " .. color.clear,
	"entering interactive mode for playing statistics\nStatistics are available through 'stats' variable. Run" ..
	"\n\t- stats:list() to get the list of column names" ..
	"\n\t- stats:top(column) to get the top 10 of selected field" ..
	"\n\t- stats:select_dump(nb) to dump 'nb' entries of stats table" ..
	"\n\t- stats:select_table({column_1, column_2, etc.}, cond_func)) to select some columns and filter them based on 'cond_func' function" ..
	"\n\nExamples:" ..
	"\n\t- stats:top('useragent')" ..
	"\n\t- stats:select_table({'ressource', 'status'}, function(elem) return elem.status == '404' end):top('ressource')")
end)


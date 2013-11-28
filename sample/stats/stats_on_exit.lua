
local stats = require('stats')

-- Run some stats at exit on collected
-- http info
haka.on_exit(function ()
	print("top 10 (by default) of useragent header")
	stats:top('useragent')
	print("")

	print("select columns 'ip, 'method' and 'resource' from the stats table")
	stats:select_table({'ip', 'method', 'resource'}):dump(5)
	print("")

	print("list of source ip using 'Mozilla/2.0' as user-gent'")
	stats:select_table({'ip', 'useragent'},
		function(elem) return elem.useragent:find('Mozilla/2.0') end):dump(5)
	print("")

	print("top ten of http resources that generated the most 404 status error")
	stats:select_table({'resource', 'status'},
		function(elem) return elem.status == '404' end):top('resource')
	print("")
end)

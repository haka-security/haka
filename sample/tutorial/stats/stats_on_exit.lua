
require('stats')

haka.on_exit(function ()
	print("top 5 of requested http host")
	stats:top('host', 5)
	print("top 10 (by default) of useragent header")
	stats:top('useragent')
    print("dump selected entries of stats table")
	stats:select_table({'method', 'host', 'ip'}):dump()
	print("top ten of http ressources that generatged the most 404 status error")
	stats:select_table({'ressource', 'status'},
	   function(elem) return elem.status == '404' end):top('ressource')
end)

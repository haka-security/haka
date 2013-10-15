
require('httpconfig')

------------------------------------
-- Transformation Methods
------------------------------------

local transformations = {

	nothing = function(uri)
		return uri
	end,

	uncomments = function(uri)
		return string.gsub(uri, '/%*.-%*/', ' ')
	end,

	nonulls = function(uri)
		return string.gsub(uri, "%z", '')
	end,

	nospaces = function(uri)
		return string.gsub(uri, "%s+", " ")
	end,

	decode = function(uri)
		uri = string.gsub (uri, '+', ' ')
		uri = string.gsub (uri, '%%(%x%x)',
			function(h) return string.char(tonumber(h,16)) end)
		return uri
	end,

	lower = function(uri)
		return uri:lower()
	end
}

------------------------------------
-- Malicious patterns
------------------------------------

local comments = { '%-%-', '#', '%z', '/%*.-%*/'}

local probing = { "^[\"'`´’‘;]", "[\"'`´’‘;]$"}

--local encodings = "([%W])'0x'[%x]^+3 ![%a]"

local sql_keywords = {	"select", "show", "top", "'distinct",
						"from", "dual", "where'", "offset",
						"order by", "group by", "having",
						"limit", "union", "rownum", "%([%s]case",
						"insert into", "drop", "update", "null"
					 }

local sql_functions = { "ascii", "char", "length", "concat",
						"substring", "substr", "benchmark", "compress",
						"load_file", "version", "uncompress",
						-- TODO to be continued ...
					  }



local location = { 'args', 'cookies' }

------------------------------------
-- SQLi Rule Group
------------------------------------

sqli = haka.rule_group {
	name = 'sqli',

	init = function (self, http)
		request = http.request
		request.cookies = http.request:split_cookies()
		request.args = http.request:split_uri().args
		request.sqli = {}
		for _, where in ipairs(location) do
			request.sqli[where] = {}
			request.sqli[where].score = 0
			request.sqli[where].msg = {}
		end
	end,

	fini = function (self, pkt)
	end,

	continue = function (self, http, ret)
		if ret then
			if ret.score >= 8  then
				haka.log.error("filter", "SQLi attack detected !!! \n%s", table.concat(ret.msg, '\n'))
				http:drop()
				return nil
			end
		end
		return true
	end
}

local function check_sqli(where, pattern, score, msg, trans)
	sqli:rule {
		hooks = { 'http-request' },
		eval = function (self, http)
			if http.request[where] then
				local sqli = http.request.sqli
				for _, val in pairs(http.request[where]) do
						for t = 1, #trans do
							val = transformations[trans[t]](val)
						end
						if val:find(pattern) then
							sqli[where].score = sqli[where].score + score
							table.insert(sqli[where].msg, msg)
						end
				end
				return sqli[where]
			end
			return nil
		end
	}
end

for loc = 1, #location do
	for key = 1, #comments do
		check_sqli(location[loc], comments[key], 4, 'SQL comments in ' .. location[loc], {'decode', 'lower'})
	end

	for key = 1, #probing do
		check_sqli(location[loc], probing[key], 2, 'SQL probing in ' .. location[loc], {'decode', 'lower'})
	end

	for key = 1, #sql_keywords do
		check_sqli(location[loc], sql_keywords[key], 4, 'SQL keywords in ' .. location[loc], {'decode', 'lower', 'uncomments', 'nospaces'})
	end

	for key = 1, #sql_functions do
		check_sqli(location[loc], "[%s'\"`´’‘%(%)]+" .. sql_functions[key] .. "[%s]*%(", 4, 'SQL function calls in ' .. location[loc], {'decode', 'lower', 'uncomments', 'nospaces'})
	end
end

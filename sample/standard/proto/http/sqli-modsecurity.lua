
------------------------------------
-- Req./Resp Transform Operations
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

	nohex = function(uri)
		--TODO
		return uri
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
-- URI Normalization
------------------------------------

local function split_uri(uri)
	local splitted = {}
	local args = {}

	-- fragment
	uri = string.gsub(uri, '#(.*)$', function(p) splitted.fragment = p return '' end)

	-- auhtority
	uri = string.gsub(uri, '^//([^/]*)', function(p) splitted.authority = p return '' end)

	-- query
	uri = string.gsub(uri, '%?(.*)', function(p) splitted.query = p return '' end)

	-- args
	if (splitted.query) then
		string.gsub(splitted.query, '([^=&]+)=([^&?]*)&?', function(p, q) args[p] = q return '' end)
		splitted.args = args
	end

	-- path
	if uri ~= '' then splitted.path = uri end

	-- authority   = [ userinfo "@" ] host [ ":" port ]
	local authority = splitted.authority
	if not authority then return splitted end
	-- userinfo
	authority = string.gsub(authority,"^([^@]*)@", function(p) splitted.userinfo = p; return "" end)
	-- port
	authority = string.gsub(authority, ":([^:]*)$", function(p) splitted.port = p; return "" end)
	-- host
	if authority ~= "" then splitted.host = authority end

	-- userinfo = user:password (deprecated usage)
	local userinfo = splitted.userinfo
	if not userinfo then return splitted end
	-- password
	userinfo = string.gsub(userinfo, ":([^:]*)$", function(p) splitted.password = p; return "" end)
	splitted.userinfo = userinfo

	return splitted
end

-- TODO
local function rebuild_uri(norm_uri)
	return norm_uri
end


-- Transform uri field and check it agaisnt pattern using regex model
function match(val, pattern, trans, regex)
	for t = 1, #trans do
		val = transformations[trans[t]](val)
	end
	if regex.find(val, pattern) then
		return true
	end
	return nil
end


------------------------------------
-- Malicious patterns
------------------------------------

local comments = { '%-%-', '#', '%z', '/%*.-%*/'}
local probing = { "^[\"'`´’‘;]", "[\"'`´’‘;]$"}
local encodings = "([%W])'0x'[%x]^+3 ![%a]"

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

local tautologies = {
    "[%a]*[%s'\"`´’‘%(%)]+('or'/'and')[(%s][+-]*[%d]+[%s]?[><=]['=']?[%s]?[+-]*[%d]+[%s'\"`´’‘%(%)]*",
    "[%a]*[%s'\"`´’‘%(%)]+('or'/'and')[(%s]['][%a]*['][%s]('like'/'between'/'>'/'>='/'='/'<'/'<=')[%s]['][%s'\"`´’‘%(%)]*"
}

local encodings = "([%W])'0x'[%x]^+3 ![%a]"



------------------------------------
-- SQLi Detection (Modsecurity-Like)
------------------------------------

local location = {'args', 'cookie'}

sqli = haka.rule_group {
	name = "sqli",

	init = function (self, http)
		request = http.request
		request.cookie = request.headers['Cookie']
		request.sqli = {}
		request.sqli.score = {cookie = 0, args = 0}
		request.sqli.msg = ''
		request.split = split_uri(http.request.uri)
		request.args = request.split.args
	end,

	fini = function (self, pkt)
	end,

	continue = function (self, http, ret)
		if ret then
			for key, value in pairs(sqli.score) do
				if value >= 8  then
					haka.log("filter", "SQLi attack detected !!!\n\tReason = %s\n\tScore = %s -> %d", ret.msg, key, value)
					http:drop()
					return nil
				end
			end
		end
		return true
	end
}

local function check_sqli(where, pattern, score, msg, trans, regex)
	sqli:rule {
		hooks = {"http-request"},
		eval = function (self, http)
			if http.request[where] then
				loc = http.request[where]
				if type(loc) == 'table'	then
					for k, val in pairs(loc) do
						if match(val, pattern, trans, regex) then
							sqli = http.request.sqli
							sqli.score[where] = sqli.score[where] + score
							if #sqli.msg ~= 0 then sqli.msg = sqli.msg .. ', ' end
							sqli.msg = sqli.msg .. msg
							return sqli
						end
					end
				else
				--TODO should be improved
					if match(loc, pattern, trans, regex) then
						sqli = http.request.sqli
						sqli.score[where] = sqli.score[where] + score
						if #sqli.msg ~= 0 then sqli.msg = sqli.msg .. ', ' end
						sqli.msg = sqli.msg .. msg
						return sqli
					end
				end
			end
			return nil
		end
	}
end


for loc = 1, #location do
	for key = 1, #comments do
		check_sqli(location[loc], comments[key], 4, 'SQL comments in ' .. location[loc], {'decode', 'lower'}, string)
	end

	for key = 1, #probing do
		check_sqli(location[loc], probing[key], 2, 'SQL probing in ' .. location[loc], {'decode', 'lower'}, string)
	end


	check_sqli(location[loc], encodings, 3, 'SQL hex encoding in ' .. location[loc], {'decode', 'lower'}, re)

	for key = 1, #tautologies do
		check_sqli(location[loc], tautologies[key], 8, 'SQL  [Or | And] tautologies in ' .. location[loc], {'decode', 'lower', 'uncomments', 'nospaces'}, re)
	end

	for key = 1, #sql_keywords do
		check_sqli(location[loc], "[%a]*[%s'\"`´’‘%(%)]*" .. sql_keywords[key] .. "[%s'\"`´’‘%(%)]+", 4, 'SQL keywords in ' .. location[loc], {'decode', 'lower', 'uncomments', 'nospaces'}, string)
	end

	for key = 1, #sql_functions do
		check_sqli(location[loc], "[%s'\"`´’‘%(%)]+" .. sql_functions[key] .. "[%s]*%(", 4, 'SQLfunction calls ' .. location[loc], {'decode', 'lower', 'uncomments', 'nospaces'}, string)
	end

end


require('httpconfig')

------------------------------------
-- Malicious patterns
------------------------------------

local comments = { '%-%-', '#', '%z', '/%*.-%*/'}

local probing = { "^[\"'`´’‘;]", "[\"'`´’‘;]$"}

local sql_keywords = { 'select','insert','update','delete', 'union',
						-- to be extended ...
					 }

local sql_functions = { 'ascii', 'char', 'length', 'concat', 'substring',
						-- to be extended ...
					  }

------------------------------------
-- SQLi Rule Group
------------------------------------

sqli = haka.rule_group {
	name = 'sqli',

	init = function (self, http)
		request = http.request
		request.cookies = http.request:split_cookies()
		request.args = http.request:split_uri().args
		request.where =  {args = {score = 0}, cookies = {score = 0}}
	end,
}

local function check_sqli(patterns, score, trans)
	sqli:rule {
		hooks = { 'http-request' },
		eval = function (self, http)
			for _, pattern in ipairs(patterns) do
				for k, v in pairs(request.where) do
					for _, val in pairs(http.request[k]) do
						for _, f in ipairs(trans) do
							val = f(val)
						end
						if val:find(pattern) then
							v.score = v.score + score
							haka.log.error('filter', 'SQLi attack detected')
							http:drop()
						end
					end
				end
			end
			return nil
		end
	}
end

check_sqli(comments, 4, { decode, lower })
check_sqli(probing, 2, { decode, lower })
check_sqli(sql_keywords, 4, { decode, lower, uncomments, nospaces })
check_sqli(sql_functions, 4, { decode, lower, uncomments, nospaces })


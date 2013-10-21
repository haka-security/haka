
require('httpconfig')
require('httpdecode')

------------------------------------
-- Malicious patterns
------------------------------------

local sql_comments = { '%-%-', '#', '%z', '/%*.-%*/' }

-- common pattern used in intial attack stage to ckeck for SQLi vulnerabilites
local probing = { "^[\"'`´’‘;]", "[\"'`´’‘;]$" }

local sql_keywords = {
	'select', 'insert', 'update', 'delete', 'union',
	-- you can extent this list with other sql keywords
}

local sql_functions = {
	'ascii', 'char', 'length', 'concat', 'substring',
	-- you can extend this list with other sql functions
}

------------------------------------
-- SQLi Rule Group
------------------------------------

-- define a security rule group related to SQLi attacks
sqli = haka.rule_group {
	name = 'sqli',
	-- initialize some values before evaluating any security rule
	init = function (self, http)
		dump_request(http)

		-- another way to split cookie header value and query's arguments
		http.sqli = {
			cookies = {
				value = http.request:split_cookies(),
				score = 0
			},
			args = {
				value = http.request:split_uri().args,
				score = 0
			}
		}
	end,
}

local function check_sqli(patterns, score, trans)
	sqli:rule {
		hooks = { 'http-request' },
		eval = function (self, http)
			for k, v in pairs(http.sqli) do
				if v.value then
					for _, val in pairs(v.value) do
						for _, f in ipairs(trans) do
							val = f(val)
						end

						for _, pattern in ipairs(patterns) do
							if val:find(pattern) then
								v.score = v.score + score
							end
						end
					end

					if v.score >= 8 then
						haka.log.error("sqli", "    SQLi attack detected in %s with score %d", k, v.score)
						http:drop()
						return
					end
				end
			end
		end
	}
end

-- generate a security rule for each malicious pattern class
-- (sql_keywords, sql_functions, etc.)
check_sqli(sql_comments, 4, { decode, lower })
check_sqli(probing, 2, { decode, lower })
check_sqli(sql_keywords, 4, { decode, lower, uncomments, nospaces })
check_sqli(sql_functions, 4, { decode, lower, uncomments, nospaces })

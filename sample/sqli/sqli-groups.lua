
require('httpconfig')
require('httpdecode')

------------------------------------
-- Malicious patterns
------------------------------------

local sql_comments = { '%-%-', '#', '%z', '/%*.-%*/' }

-- Common pattern used in initial attack stage to check for SQLi vulnerabilities
local probing = { "^[\"'`´’‘;]", "[\"'`´’‘;]$" }

local sql_keywords = {
	'select', 'insert', 'update', 'delete', 'union',
	-- You can extent this list with other sql keywords
}

local sql_functions = {
	'ascii', 'char', 'length', 'concat', 'substring',
	-- You can extend this list with other sql functions
}

------------------------------------
-- SQLi Rule Group
------------------------------------

-- Define a security rule group related to SQLi attacks
sqli = haka.rule_group{
	hook = httplib.events.request,
	name = 'sqli',
	-- Initialize some values before evaluating any security rule
	init = function (http, request)
		dump_request(request)

		-- Another way to split cookie header value and query's arguments
		http.sqli = {
			cookies = {
				value = request.split_cookies,
				score = 0
			},
			args = {
				value = request.split_uri.args,
				score = 0
			}
		}
	end,
}

local function check_sqli(patterns, score, trans)
	sqli:rule(
		function (http, request)
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
						haka.alert{
							description = string.format("SQLi attack detected in %s with score %d", k, v.score),
							severity = 'high',
							confidence = 'medium',
							sources = haka.alert.address(http.flow.srcip),
							targets = {
								haka.alert.address(http.flow.dstip),
								haka.alert.service(string.format("tcp/%d", http.flow.dstport), "http")
							},
						}

						http:drop()
						return
					end
				end
			end
		end
	)
end

-- Generate a security rule for each malicious pattern class
-- (sql_keywords, sql_functions, etc.)
check_sqli(sql_comments, 4, { decode, lower })
check_sqli(probing, 2, { decode, lower })
check_sqli(sql_keywords, 4, { decode, lower, uncomments, nospaces })
check_sqli(sql_functions, 4, { decode, lower, uncomments, nospaces })

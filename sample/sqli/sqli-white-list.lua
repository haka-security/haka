
require('httpconfig')
require('httpdecode')

------------------------------------
-- Malicious patterns
------------------------------------

local sql_comments = { '%-%-', '#', '%z', '/%*.-%*/' }

local probing = { "^[\"'`´’‘;]", "[\"'`´’‘;]$" }

local sql_keywords = {
	'select', 'insert', 'update', 'delete', 'union',
	-- You can extend this list with other sql keywords
}

local sql_functions = {
	'ascii', 'char', 'length', 'concat', 'substring',
	-- You can extend this list with other sql functions
}

------------------------------------
-- White List resources
------------------------------------

local safe_resources = {
	'/foo/bar/safepage.php', '/action.php',
	-- You can extend this list with other white list resources
}

------------------------------------
-- SQLi Rule Group
------------------------------------

sqli = haka.rule_group{
	name = 'sqli',
	-- Initialisation
	init = function (self, http)
		dump_request(http)

		-- Another way to split cookie header value and query's arguments
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

	-- Continue will be executed after evaluation of
	-- each security rule.
	-- Here we check the return value ret to decide
	-- if we skip the evaluation of the rest of the
	-- rule.
	continue = function (self, http, ret)
		return not ret
	end
}

------------------------------------
-- SQLi White List Rule
------------------------------------

sqli:rule{
	hooks = { 'http-request' },
	eval = function (self, http)
		-- Split uri into subparts and normalize it
		local splitted_uri = http.request:split_uri():normalize()
		for	_, res in ipairs(safe_resources) do
			-- Skip evaluation if the normalized path (without dot-segments)
			-- is in the list of safe resources
			if splitted_uri.path == res then
				haka.log("sqli", "skip SQLi detection (white list rule)")
				return true
			end
		end
	end
}

------------------------------------
-- SQLi Rules
------------------------------------

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
						local conn = http.connection
						-- Report an alert (long format)
						haka.alert{
							description = string.format("SQLi attack detected in %s with score %d", k, v.score),
							severity = 'high',
							confidence = 'high',
							method = {
								description = "SQL Injection Attack",
								ref = "cwe-89"
							},
							sources = haka.alert.address(conn.srcip),
							targets = {
								haka.alert.address(conn.dstip),
								haka.alert.service(string.format("tcp/%d", conn.dstport), "http")
							},
						}
						http:drop()
						return
					end
				end
			end
		end
	}
end

check_sqli(sql_comments, 4, { decode, lower })
check_sqli(probing, 2, { decode, lower })
check_sqli(sql_keywords, 4, { decode, lower, uncomments, nospaces })
check_sqli(sql_functions, 4, { decode, lower, uncomments, nospaces })

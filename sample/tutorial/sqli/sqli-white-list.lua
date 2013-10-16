
require('httpconfig')
require('httpdecode')

------------------------------------
-- Malicious patterns
------------------------------------

local sqli_comments = { '%-%-', '#', '%z', '/%*.-%*/' }

local probing = { "^[\"'`´’‘;]", "[\"'`´’‘;]$" }

local sql_keywords = { 'select', 'insert', 'update', 'delete', 'union',
	-- you can extend this list with other sql keywords
}

local sql_functions = { 'ascii', 'char', 'length', 'concat', 'substring',
	-- you can extend this list with other sql functions
}

------------------------------------
-- White List ressources
------------------------------------

local safe_ressources = { 
	'/site/page1', '/site/page2',
	-- you can extend this list with other white list ressources
}

------------------------------------
-- SQLi Rule Group
------------------------------------

sqli = haka.rule_group {
	name = 'sqli',
	-- initialisation
	init = function (self, http)
		request = http.request
		request.cookies = http.request:split_cookies()
		request.args = http.request:split_uri().args
		request.where =  {args = {score = 0}, cookies = {score = 0}}
	end,

	-- continue will be executed after evaluation of
	-- each security rule.
	-- here we check the return value ret to decide
	-- if we skip the evaluation of the rest of the
	-- rule.
	continue = function (self, http, ret)
		if ret and ret == -1 then return false end
	end
}

------------------------------------
-- SQLi White List Rule
------------------------------------

sqli:rule {
	hooks = { 'http-request' },
	eval = function (self, http)
		-- split uri into subparts and normalize it
		local splitted_uri = http.request:split_uri():normalize()
		for	_, res in ipairs(safe_ressources) do
			-- skip evaluation if the normalized path (without dot-segments)
			-- is in the list of safe ressources
			if splitted_uri.path == res then
				haka.log.warning("filter", "Skip SQLi detection (White list rule)")
				return -1
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
			for k, v in pairs(request.where) do
				for _, val in pairs(http.request[k]) do
					for _, pattern in ipairs(patterns) do
						for _, f in ipairs(trans) do
							val = f(val)
						end
						if val:find(pattern) then
							v.score = v.score + score
							if v.score >= 8 then
								haka.log.error("filter", "SQLi attack detected !!!")
								http:drop()
							end
						end
					end
				end
			end
		end
	}
end

check_sqli(comments, 4, { decode, lower })
check_sqli(probing, 2, { decode, lower })
check_sqli(sql_keywords, 4, { decode, lower, uncomments, nospaces })
check_sqli(sql_functions, 4, { decode, lower, uncomments, nospaces })


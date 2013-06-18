
------------------------------------
-- Loading disscetors
------------------------------------

require("ipv4")
require("tcp")
require("http")


------------------------------------
-- Function definition
------------------------------------

local function getpayload(data)
	payload = ''
	for i = 1, #data do
		payload = payload .. string.char(data[i])
	end
	return payload
end


local function contains(table, elem)
	return table[elem] ~= nil
end

local function dict(table)
	local ret = {}
	for _, v in pairs(table) do
		ret[v] = true
	end
	return ret
end


------------------------------------
-- IP compliance
------------------------------------

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		-- bad IP checksum
		if not pkt:verify_checksum() then
			haka.log.error("filter", "Bad IP checksum")
			pkt:drop()
		end
	end
}

------------------------------------
-- IP attacks
------------------------------------

haka.rule {
	hooks = { "ipv4-up" },
	eval = function (self, pkt)
		if pkt.src == pkt.dst then
			haka.log.error("filter", "Land attack detected")
			pkt:drop()
		end
	end
}

------------------------------------
-- TCP attacks
------------------------------------

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		--Xmas scan, as made by nmap -sX <IP>
		if pkt.flags.psh and pkt.flags.fin and pkt.flags.urg then
			haka.log.error("filter", "Xmas attack detected !!!")
			pkt:drop()
		end
	end
}

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		local bindshell = "\xeb\x1f\x5e\x89\x76\x08\x31\xc0\x88\x46\x07\x89\x46\x0c\xb0\x0b" ..
						  "\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xcd\x80\x31\xdb\x89\xd8\x40\xcd" ..
						  "\x80\xe8\xdc\xff\xff\xff/bin/sh"
		if #pkt.payload > 0 then
			-- reconstruct payload
			payload = getpayload(pkt.payload)
			-- test if shellcode is present in data
			if string.find(payload, bindshell) then
				haka.log.error("filter", "/bin/sh shellcode detected !!!")
				pkt:drop()
			end
		end
	end
}

------------------------------------
-- HTTP compliance
------------------------------------
-- check http method value
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local http_methods = dict({ 'get', 'post', 'head', 'put', 'trace', 'delete', 'options' })
		local method = http.request.method:lower()
		if not contains(http_methods, method) then
			haka.log.error("filter", "non authorized http method '%s'", method)
			http:drop()
		end
	end
}

-- check http version value
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local http_versions = dict({ '0.9', '1.0', '1.1' })
		local protocol = http.request.version:sub(1,4)
		local version = http.request.version:sub(6)
		if not protocol == "HTTP" or not contains(http_versions, version) then
			haka.log.error("filter", "unsupported http version '%s/%s'", protocol, version)
			http:drop()
		end
	end
}

-- check content length value
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local content_length = http.request.headers["Content-Length"]
		if content_length then
			content_length = tonumber(content_length)
			if content_length == nil or content_length < 0 then
				haka.log.error("filter", "corrupted content-length header value")
				http:drop()
			end
		end
	end
}

------------------------------------
-- HTTP Attacks
------------------------------------

-- detect malicious web scanners
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		--user-agent patterns of known web scanners
		local http_useragent = { 
			nikto	= '.+%(Nikto%/.+%)%s%(Evasions:.+%)%s%(Test:.+%)',
			nessus	= '^Nessus.*',
			w3af	= '*.%s;w3af.sf.net%)',
			sqlmap	= '^sqlmap%/.*%s%(http:%/%/sqlmap.*%)',
			fimap	= '^fimap%.googlecode%.%com.*',
			grabber	= '^Grabber.*'
		}

		if (http.request.headers['User-Agent']) then
			local user_agent = http.request.headers['User-Agent']
			for scanner, pattern in pairs(http_useragent) do
				if user_agent:match(pattern) then
					haka.log.error("filter", "'%s' scan detected !!!", scanner)
					http:drop()
				end
			end
		end
	end
}

------------------------------------
-- HTTP Policy
------------------------------------

-- add custom user-agent
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		http.request.headers["User-Agent"] = "Haka User-Agent"
	end
}

-- allow only get and post methods
haka.rule {
	hooks = {"http-request"},
	eval = function (self, http)
		local method = http.request.method:lower()
		if method ~= 'get' and method ~= 'post' then
			haka.log.warning("filter", "forbidden http method '%s'", method)
		end
	end
}

------------------------------------
-- Firewall rules
------------------------------------

akpf = haka.rule_group {
	name = "akpf",
	init = function (self, pkt)
		haka.log.debug("filter", "entering packet filetring rules : %d --> %d", pkt.tcp.srcport, pkt.tcp.dstport)
	end,
	fini = function (self, pkt)
		haka.log.error("filter", "packet dropped : drop by default")
		pkt:drop()
	end,
	continue = function (self, ret)
		return not ret
	end
}


akpf:rule {
	hooks = {"tcp-connection-new"},
	eval = function (self, pkt)

		local netsrc = ipv4.network("10.2.96.0/22")
		local netdst = ipv4.network("10.2.104.0/22")

		local tcp = pkt.tcp

		if netsrc:contains(tcp.ip.src) and
		   netdst:contains(tcp.ip.dst) and
		   tcp.dstport == 80 then

			haka.log.warning("filter", "authorizing http traffic")
			pkt.next_dissector = "http"
			return true
		end
	end
}

akpf:rule {
	hooks = {"tcp-connection-new"},
	eval = function (self, pkt)

		local netsrc = ipv4.network("10.2.96.0/22")
		local netdst = ipv4.network("10.2.104.0/22")

		local tcp = pkt.tcp

		if netsrc:contains(tcp.ip.src) and
		   netdst:contains(tcp.ip.dst) and
		   tcp.dstport == 22 then

			haka.log.warning("filter", "authorizing ssh traffic")
			haka.log.warning("filter", "no available dissector for ssh")
			return true
		end
	end
}


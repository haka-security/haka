--------------------------
-- Loading dissectors
--------------------------

require('protocol/ipv4')
require('protocol/tcp')
require('protocol/http')

require('tblutils')

stats = {}
local mt_stats = {}
mt_stats.__index = mt_stats
setmetatable(stats, mt_stats)

--------------------------
-- Setting next dissector
--------------------------

haka.rule {
    hooks = { "tcp-connection-new" },
    eval = function(self, pkt)
        local tcp = pkt.tcp
		if tcp.dstport == 80 then
			pkt.next_dissector = "http"
		end
    end
}

--------------------------
-- Recording http info
--------------------------

haka.rule {
	hooks = { 'http-response' },
	eval = function (self, http)
		local conn = http.connection
		local response = http.response
		local request = http.request
		local split_uri = request:split_uri():normalize()
		local entry = {}
		entry.ip = tostring(conn.srcip)
		entry.method = request.method
		entry.ressource = split_uri.path or ''
		entry.host = split_uri.host or ''
		entry.useragent = request.headers['User-Agent'] or ''
		entry.referer = request.headers['Referer'] or ''
		entry.status = response.status
		table.insert(stats, entry)
	end
}


function mt_stats:select_table(elements, where)
	local ret = {}
	for _, entry in ipairs(self) do
		local selected_entry = {}
		if not where or where(entry) then
			for _, field in ipairs(elements) do
				selected_entry[field] = entry[field]
			end
			table.insert(ret, selected_entry)
		end
	end
	setmetatable(ret, mt_stats)
	return ret
end

function mt_stats:top(field, nb)
	nb = nb or 10
	assert(field, "missing field parameter")
	local grouped = group_by(self, field)
	local sorted = order_by(grouped, function(p, q) return #p[2] > #q[2] end)
	local iter = 1
	for _, entry in ipairs(sorted) do
		if iter <= nb then
			print('\t', entry[1], '(', #entry[2], ')')
		else
			break
		end
		iter = iter + 1
	end
end

function mt_stats:dump(nb)
	local max = self:list()
	nb = nb or #self
	local iter = 1
	for _, entry in ipairs(self) do
		if iter <= nb then
			local content = {}
			for k, v in pairs(entry) do
				table.insert(content, string.format("%-" .. tostring(max[k]) .. "s", v))
			end
			print("| " .. table.concat(content, " | ") .. " |")
		else
			break
		end
		iter = iter + 1
	end
end

function mt_stats:list()
	local max = max_length_field(self)
	if #self > 0 then
		local columns = {}
		for k, _ in pairs(self[1]) do
			table.insert(columns, string.format("%-" .. tostring(max[k]) .. "s", k))
		end
	print("| " .. table.concat(columns, " | ") .. " |")
	end
	return max
end


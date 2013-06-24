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


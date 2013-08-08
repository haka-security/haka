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

local function string_convert(t)
	for i=1,#t do
		if type(t[i]) == "number" then t[i] = string.char(t[i]) end
	end
	return table.concat(t)
end

local bindshell = string_convert({0xeb, 0x1f, 0x5e, 0x89, 0x76, 0x08, 0x31, 0xc0,
	0x88, 0x46, 0x07, 0x89, 0x46, 0x0c, 0xb0, 0x0b, 0x89, 0xf3, 0x8d, 0x4e, 0x08,
	0x8d, 0x56, 0x0c, 0xcd, 0x80, 0x31, 0xdb, 0x89, 0xd8, 0x40, 0xcd, 0x80, 0xe8,
	0xdc, 0xff, 0xff, 0xff, "/bin/sh"})

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
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

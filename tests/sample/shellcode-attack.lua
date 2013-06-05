require("ipv4")
require("tcp")

local bindshell = "\xeb\x1f\x5e\x89\x76\x08\x31\xc0\x88\x46\x07\x89\x46\x0c\xb0\x0b" ..
                  "\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xcd\x80\x31\xdb\x89\xd8\x40\xcd" ..
                  "\x80\xe8\xdc\xff\xff\xff/bin/sh"

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		if #pkt.payload > 0 then
			-- reconstruct payload
			payload = ''
			for i = 1, #pkt.payload do
				payload = payload .. string.format("%c", pkt.payload[i])
			end
			-- test if shellcode is present in data
			if string.find(payload, bindshell) then
				haka.log("filter", "/bin/sh shellcode detected !!!")
				pkt:drop()
			end
		end
	end
}


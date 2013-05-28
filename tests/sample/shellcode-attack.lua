require("proto-ipv4")
require("proto-tcp")

local bindshell =	"\xeb\x1f\x5e\x89\x76\x08\x31\xc0\x88\x46\x07\x89\x46\x0c\xb0\x0b" ..
					"\x89\xf3\x8d\x4e\x08\x8d\x56\x0c\xcd\x80\x31\xdb\x89\xd8\x40\xcd" ..
					"\x80\xe8\xdc\xff\xff\xff/bin/sh"

return function (pkt)
	local ip_h = ipv4(pkt)
	if ip_h and ip_h.proto == 6 then
		tcp_h = tcp(ip_h)
		if tcp_h and #tcp_h.payload > 0 then
				payload = ""
				for i = 1, #tcp_h.payload do 
					payload = payload .. string.format("%c", tcp_h.payload[i])
				end
				-- printing payload
				print(payload)
				if(string.find(payload, bindshell)) then
					log("filter", "/bin/sh shellcode detected !!!")
					return packet.DROP
				end
		end
	end
	return packet.ACCEPT
end


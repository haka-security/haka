
ipv4 = require('proto-ipv4')

return function(pkt)
    ip = ipv4(pkt)

    if ip.dst == ipv4.addr("10.2.253.137") then
		ip.dst = ipv4.addr("192.168.1.137")
    end
	
end


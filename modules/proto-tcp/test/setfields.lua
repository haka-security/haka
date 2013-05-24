-- Basic test that will output some basic information about the
-- received packets.

function bool(val)
	if(val) then
		return 1, "Set"
	end
	return 0, "Not set"
end

function checks(proto)
	if proto:verifyChecksum() then
		good = "[Good: True]"
		bad = "[Bad: False]"
	else
		good = "[Good: False]"
		bad = "[Bad: True]"
	end
return good, bad
end 

ipv4 = require("proto-ipv4")
tcp = require("proto-tcp")

return function(pkt)

	local ip_h = ipv4(pkt)
	local tcp_h = tcp(ip_h)

	local good, bad = checks(ip_h)

	tcp_h.srcport = 3
    tcp_h.dstport = 65535
	tcp_h.seq = 123456
	tcp_h.ack_seq = 654321
    tcp_h.res = 0
    tcp_h.hdr_len = 20
	--tcp_h.flags.all))
    tcp_h.flags.fin = true
	tcp_h.flags.syn = true
    tcp_h.flags.rst = true
    tcp_h.flags.psh = true
    tcp_h.flags.ack = true
    tcp_h.flags.urg = true
    tcp_h.flags.ecn = true
    tcp_h.flags.cwr = true
    tcp_h.window_size = 32
    tcp_h.urgent_pointer = 15

	tcp_h:computeChecksum()

	return packet.ACCEPT

end

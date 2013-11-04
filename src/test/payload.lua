-- Basic test that play with function sub, left and right
require('protocol/ipv4' )
require('protocol/icmp' )

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		pkt.payload:sub(0,40):setstring("0123456789abcdeABCDE0123456789abcdeABCDE01234567")
		local left = pkt.payload:left(50):asstring()
		print("V=", left)
	end
}

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		pkt.payload:sub(0,48):setfixedstring("0123456789abcdefghijklmnopqrstuvwxyz0123456789AB")
		local left = pkt.payload:left(50):asstring()
		print("V=", left)
	end
}

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		local substring = pkt.payload:sub(40,16):asstring()
		print("V=", substring)
	end
}

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		local right = pkt.payload:right(48):asstring()
		print("V=", right)
	end
}

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		pkt.payload:erase(40,8)
		local remain = pkt.payload:sub(40,8):asstring()
		print("V=", remain)
	end
}

haka.rule {
	hook = haka.event('icmp', 'receive_packet'),
	eval = function (pkt)
		pkt.payload:erase(0,40)
		local remain = pkt.payload:right(0):asstring()
		print("V=", remain)
	end
}


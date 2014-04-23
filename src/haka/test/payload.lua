-- Basic test that play with function sub, left and right
local ipv4 = require('protocol/ipv4' )

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		pkt.payload:sub(0, 40):setstring("0123456789abcdeABCDE0123456789abcdeABCDE01234567")
		local left = pkt.payload:sub(0, 50):asstring()
		print("V=", left)
	end
}

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		pkt.payload:sub(0, 48):setfixedstring("0123456789abcdefghijklmnopqrstuvwxyz0123456789AB")
		local left = pkt.payload:sub(0, 50):asstring()
		print("V=", left)
	end
}

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		local substring = pkt.payload:sub(40, 16):asstring()
		print("V=", substring)
	end
}

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		local right = pkt.payload:sub(48):asstring()
		print("V=", right)
	end
}

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		pkt.payload:sub(40, 8):erase()
		local remain = pkt.payload:sub(40, 8):asstring()
		print("V=", remain)
	end
}

haka.rule {
	hook = ipv4.events.receive_packet,
	eval = function (pkt)
		pkt.payload:sub(0, 40):erase()
		local remain = pkt.payload:sub(0):asstring()
		print("V=", remain)
	end
}


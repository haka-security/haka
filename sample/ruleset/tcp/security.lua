local rem = require("regexp/pcre")

------------------------------------
-- TCP attacks
------------------------------------

haka.rule {
	hook = tcp.events.receive_packet,
	eval = function (pkt)
		--Xmas scan, as made by nmap -sX <IP>
		if pkt.flags.psh and pkt.flags.fin and pkt.flags.urg then
			haka.alert{
				description = "Xmas attack detected",
				sources = haka.alert.address(pkt.ip.src),
				targets = haka.alert.address(pkt.ip.dst)
			}
			pkt:drop()
		end
	end
}

local shellcode = "%xeb %x1f %x5e %x89 %x76 %x08 %x31 %xc0" ..
	"%x88 %x46 %x07 %x89 %x46 %x0c %xb0 %x0b" ..
	"%x89 %xf3 %x8d %x4e %x08 %x8d %x56 %x0c" .. 
	"%xcd %x80 %x31 %xdb %x89 %xd8 %x40 %xcd" ..
	"%x80 %xe8 %xdc %xff %xff %xff /bin/sh"

local re = rem.re:compile(shellcode, rem.re.EXTENDED)

haka.rule {
	hook = tcp_connection.events.receive_data,
	options = {
		streamed = true,
	},
	eval = function (flow, iter)
		-- match malicious pattern across multiple packets
		local ret = re:match(iter)
		if ret then
			haka.alert{
				description = "/bin/sh shellcode detected",
				sources = haka.alert.address(flow.srcip),
				targets = haka.alert.address(flow.dstip)
			}
			flow:drop()
		end
	end
}

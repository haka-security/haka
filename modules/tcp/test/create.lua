-- Test that will duplicate a tcp connection

require("ipv4")
require("tcp")

haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, pkt)
		if pkt.dstport == 4444 or pkt.srcport == 4444 then
			local npkt = haka.packet.new()
			npkt = ipv4.create(npkt)
			npkt.ttl = pkt.ip.ttl
			npkt.dst = pkt.ip.dst
			npkt.src = pkt.ip.src
			npkt = tcp.create(npkt)
			npkt.window_size = pkt.window_size
			npkt.seq = pkt.seq+1000
			if pkt.ack_seq ~= 0 then
				npkt.ack_seq = pkt.ack_seq+1000
			end
			npkt.flags.all = pkt.flags.all
			npkt.dstport = pkt.dstport+10
			npkt.srcport = pkt.srcport+10
			npkt.payload:resize(#pkt.payload)
			for i=1,#pkt.payload do
				npkt.payload[i] = pkt.payload[i]
			end
			npkt:send()
		end
	end
}

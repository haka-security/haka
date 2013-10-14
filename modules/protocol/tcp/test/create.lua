-- Test that will duplicate a tcp connection

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/tcp-connection")

haka.rule {
	hook = haka.event('tcp', 'receive_packet'),
	eval = function (pkt)
		if pkt.dstport == 4444 or pkt.srcport == 4444 then
			local npkt = haka.dissector.get('raw').create()
			npkt = haka.dissector.get('ipv4').create(npkt)
			npkt.ttl = pkt.ip.ttl
			npkt.dst = pkt.ip.dst
			npkt.src = pkt.ip.src
			npkt = haka.dissector.get('tcp').create(npkt)
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
			npkt:inject()
		end
	end
}

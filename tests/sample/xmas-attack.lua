-- How to detect Xmas attacks

require("ipv4");
require("tcp");
haka.rule {
	hooks = { "tcp-up" },
	eval = function (self, tcp_h)
		--Xmas scan, as made by nmap -sX <IP>
		if tcp_h.flags.psh and tcp_h.flags.fin and tcp_h.flags.urg then
			haka.log.debug("filter","Xmas attack!! Details : ");
			haka.log.debug("filter","    IP src: %s ; port src: %s",tcp_h.ip.src,tcp_h.srcport);
			haka.log.debug("filter","    IP dst: %s ; port dst: %s",tcp_h.ip.dst,tcp_h.dstport);
		end
	end
}

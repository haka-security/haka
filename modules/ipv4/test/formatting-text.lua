
-- Here we test different way of formatting text
-- Both way should write the same output

require("ipv4")

haka.rule {
	hooks = {"ipv4-up"},
	eval = function (self,pkt)
		print(string.format("SRC:%s - DST:%s", pkt.src, pkt.dst))
		print(string.format("SRC:" .. pkt.src .. " - DST:" .. pkt.dst))
	end
}

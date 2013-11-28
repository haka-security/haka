
require('protocol/ipv4')

haka.rule{
	hooks = { "ipv4-up" },
	eval = function (self, p)
		pkt = p
		require('alert-doc')
		require('alertupdate-doc')
	end
}

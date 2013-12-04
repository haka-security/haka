-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/ipv4")
require("protocol/tcp")
require("protocol/http")

haka.rule {
	hooks = { "tcp-connection-new" },
	eval = function (self, pkt)
		if pkt.tcp.dstport == 80 then
			pkt.next_dissector = "http"
		end
	end
}

haka.rule {
	hooks = { 'http-request' },
	eval = function (self, pkt)
		local uri = pkt.request.uri
		print('--> splitting uri')
		splitted = pkt.request:split_uri()
		print(splitted)
	end
}

haka.rule {
	hooks = { 'http-request' },
	eval = function (self, pkt)
		local uri = pkt.request.uri
		print('--> splitting uri')
		splitted2 = pkt.request:split_uri()
		print(splitted2)
		--Check that tables are the same
		--Same in the way that this is the same memory, not only the same content 
		if splitted == splitted2
		then
			print("Table are the same")
		else
			print("Table are different")
		end
	end
}


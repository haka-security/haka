-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/http")

haka.rule {
	hook = haka.dissectors.http.events.request,
	eval = function (http, request)
		local uri = request.uri
		print('--> splitting uri')
		splitted = request.split_uri
		print(splitted)
	end
}

haka.rule {
	hook = haka.dissectors.http.events.request,
	eval = function (http, request)
		local uri = request.uri
		print('--> splitting uri')
		local splitted2 = request.split_uri
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


-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("protocol/dns")

local function alert_pdns(array)
	for _, a in ipairs(array) do
		if a.type == "A" then
			haka.alert{
				description = string.format("PDNS: %s -> %s", a.name, a.ip),
			}
		end
	end
end

haka.rule {
	on = haka.dissectors.dns.events.response,
	eval = function (dns, response)
		alert_pdns(response.answer)
		alert_pdns(response.additional)
	end
}

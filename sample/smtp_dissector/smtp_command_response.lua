-- This Source Code Form is subject to the terms of the Mozilla Public
-- License, v. 2.0. If a copy of the MPL was not distributed with this
-- file, You can obtain one at http://mozilla.org/MPL/2.0/.

require("smtp")

haka.rule {
	hook = haka.dissectors.smtp.events.command,
	eval = function (flow, command)
		print("SMTP COMMAND")
		debug.pprint(command, nil, nil, { debug.hide_underscore, debug.hide_function })
	end
}

haka.rule {
	hook = haka.dissectors.smtp.events.response,
	eval = function (flow, response)
		print("SMTP RESPONSE")
		debug.pprint(response, nil, nil, { debug.hide_underscore, debug.hide_function })
	end
}
